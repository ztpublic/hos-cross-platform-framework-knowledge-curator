/**
 * @file ohosFileSystemWatcher.cpp
 * @author MiKTeX HarmonyOS Port
 * @brief File system watcher (HarmonyOS) - polling-based, no inotify
 *
 * @copyright Copyright © 2021-2024 Christian Schenk
 *
 * This file is part of the MiKTeX Core Library.
 *
 * The MiKTeX Core Library is licensed under GNU General Public License version
 * 2 or any later version.
 */

#include "config.h"

#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "internal.h"

#include "ohosFileSystemWatcher.h"

using namespace std;

using namespace MiKTeX::Core;
using namespace MiKTeX::Util;

unique_ptr<FileSystemWatcher> FileSystemWatcher::Create()
{
    return make_unique<ohosFileSystemWatcher>();
}

ohosFileSystemWatcher::ohosFileSystemWatcher()
{
    if (pipe(cancelPipe) != 0)
    {
        MIKTEX_FATAL_CRT_ERROR("pipe");
    }
    // Set read end non-blocking so select() won't hang
    int flags = fcntl(cancelPipe[0], F_GETFL, 0);
    if (flags >= 0)
    {
        fcntl(cancelPipe[0], F_SETFL, flags | O_NONBLOCK);
    }
}

ohosFileSystemWatcher::~ohosFileSystemWatcher()
{
    try
    {
        Stop();
        if (cancelPipe[0] >= 0)
        {
            close(cancelPipe[0]);
            cancelPipe[0] = -1;
        }
        if (cancelPipe[1] >= 0)
        {
            close(cancelPipe[1]);
            cancelPipe[1] = -1;
        }
    }
    catch (const exception&)
    {
    }
}

void ohosFileSystemWatcher::AddDirectories(const vector<PathName>& directories)
{
    unique_lock<shared_mutex> l(mutex);
    for (const auto& dir : directories)
    {
        bool alreadyWatched = false;
        for (const auto& existing : watchDirs)
        {
            if (existing == dir)
            {
                alreadyWatched = true;
                break;
            }
        }
        if (alreadyWatched)
        {
            continue;
        }
        trace_files->WriteLine("core", fmt::format("watching directory: {0}", Q_(dir.ToDisplayString())));
        watchDirs.push_back(dir);
        // Take initial snapshot
        snapshots[dir.ToString()] = ScanDirectory(dir);
    }
}

bool ohosFileSystemWatcher::Start()
{
    bool runningExpected = false;
    if (!running.compare_exchange_strong(runningExpected, true))
    {
        return false;
    }
    StartThreads();
    return true;
}

bool ohosFileSystemWatcher::Stop()
{
    bool runningExpected = true;
    if (!running.compare_exchange_strong(runningExpected, false))
    {
        return false;
    }
    // Wake up the polling thread via cancel pipe
    if (cancelPipe[1] >= 0)
    {
        char buf[1] = { 0 };
        (void)write(cancelPipe[1], buf, 1);
    }
    StopThreads();
    return true;
}

void ohosFileSystemWatcher::WatchDirectories()
{
    while (!done)
    {
        // Use poll() with timeout to implement timed polling, also listen for cancel
        struct pollfd pfd = {};
        pfd.fd = cancelPipe[0];
        pfd.events = POLLIN;

        int timeoutMs = static_cast<int>(pollInterval.count()) * 1000;
        int ret = poll(&pfd, (cancelPipe[0] >= 0) ? 1 : 0, timeoutMs);

        if (done) return;

        if (ret < 0)
        {
            // Interrupted by signal – retry
            if (errno == EINTR)
            {
                continue;
            }
            // On OHOS, poll may fail with EBADF if cancelPipe was closed
            if (errno == EBADF)
            {
                return;
            }
            MIKTEX_FATAL_CRT_ERROR("poll");
        }

        // Cancel pipe signalled
        if (ret > 0 && (pfd.revents & POLLIN))
        {
            return;
        }

        // Timeout expired: poll all watched directories
        {
            shared_lock<shared_mutex> l(mutex);
            vector<PathName> dirs = watchDirs; // copy under lock
            l.unlock();

            bool anyChange = false;
            for (const auto& dir : dirs)
            {
                auto newSnap = ScanDirectory(dir);
                const string key = dir.ToString();
                auto& oldSnap = snapshots[key];
                DiffAndNotify(dir, oldSnap, newSnap);
                if (oldSnap != newSnap)
                {
                    oldSnap = std::move(newSnap);
                    anyChange = true;
                }
            }
            if (anyChange)
            {
                notifyCondition.notify_all();
            }
        }
    }
}

unordered_map<string, OhosFileEntry>
ohosFileSystemWatcher::ScanDirectory(const PathName& dir)
{
    unordered_map<string, OhosFileEntry> result;
    DIR* dp = opendir(dir.GetData());
    if (dp == nullptr)
    {
        // Directory may not exist yet – return empty snapshot
        return result;
    }
    struct dirent* de;
    while ((de = readdir(dp)) != nullptr)
    {
        const char* name = de->d_name;
        if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
        {
            continue;
        }
        PathName fullPath = dir;
        fullPath /= name;
        struct stat st;
        if (lstat(fullPath.GetData(), &st) == 0)
        {
            OhosFileEntry e;
            e.mtime = st.st_mtime;
            e.size  = st.st_size;
            result[name] = e;
        }
    }
    closedir(dp);
    return result;
}

void ohosFileSystemWatcher::DiffAndNotify(
    const PathName& dir,
    const unordered_map<string, OhosFileEntry>& before,
    const unordered_map<string, OhosFileEntry>& after)
{
    // Check for added or modified files
    for (const auto& kv : after)
    {
        auto it = before.find(kv.first);
        FileSystemChangeEvent ev;
        ev.fileName = dir;
        ev.fileName /= kv.first;
        if (it == before.end())
        {
            ev.action = FileSystemChangeAction::Added;
        }
        else if (it->second.mtime != kv.second.mtime || it->second.size != kv.second.size)
        {
            ev.action = FileSystemChangeAction::Modified;
        }
        else
        {
            continue;
        }
        lock_guard<std::mutex> l(notifyMutex);
        pendingNotifications.push_back(ev);
    }

    // Check for removed files
    for (const auto& kv : before)
    {
        if (after.find(kv.first) == after.end())
        {
            FileSystemChangeEvent ev;
            ev.action = FileSystemChangeAction::Removed;
            ev.fileName = dir;
            ev.fileName /= kv.first;
            lock_guard<std::mutex> l(notifyMutex);
            pendingNotifications.push_back(ev);
        }
    }
}
