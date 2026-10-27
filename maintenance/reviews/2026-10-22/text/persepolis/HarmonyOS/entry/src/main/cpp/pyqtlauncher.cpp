/*
 * git-cola OHOS PyQt5 Launcher
 * Based on 5.15demo pyqtlauncher pattern (proven working)
 */

#include <string>
#include <Python.h>

#define LOG_DOMAIN 0x0501
#define LOG_TAG "pyqtloader"
#include "hilog-easy.h"

static std::string gPythonAppPath = "/data/storage/el1/bundle/entry/resources/resfile/persepolis/launcher.py";

/* Python C module for hilog output */
static PyObject* py_hilog_print(PyObject* self, PyObject* args) {
    const char* msg = NULL;
    int level = 1;
    if (PyArg_ParseTuple(args, "s|i", &msg, &level)) {
        if (msg) {
            if (level >= 4) LOG_E("py: %s", msg);
            else LOG_I("py: %s", msg);
        }
    }
    Py_RETURN_NONE;
}
static PyMethodDef hilog_methods[] = {
    {"hilog_print", py_hilog_print, METH_VARARGS, "Print to hilog"},
    {NULL, NULL, 0, NULL}
};
static struct PyModuleDef hilog_module = {
    PyModuleDef_HEAD_INIT, "_ohos_hilog", "OHOS Hilog", -1, hilog_methods
};
static PyObject* PyInit__ohos_hilog(void) {
    return PyModule_Create(&hilog_module);
}

struct PythonRuntimeGuard {
    PythonRuntimeGuard(const PythonRuntimeGuard&) = delete;
    PythonRuntimeGuard& operator=(const PythonRuntimeGuard&) = delete;

    PythonRuntimeGuard() {
        if (!Py_IsInitialized()) {
            /* PYTHONHOME points to resfile (has .py files), PYTHONPATH adds libs paths (has .so files) */
            setenv("PYTHONHOME", "/data/storage/el1/bundle/entry/resources/resfile/Python", 1);
            /* P015 (startup cost): the bundle resfile tree is READ-ONLY, so
             * Python can never write __pycache__ next to the .py sources and
             * recompiles every module on every launch (~1.5s for the ~13k
             * stdlib files + yt_dlp + app code). Redirect the bytecode cache
             * to the writable app sandbox: first launch populates it,
             * subsequent launches load the cached .pyc directly. */
            setenv("PYTHONPYCACHEPREFIX", "/data/storage/el2/base/files/pycache", 1);
            setenv("PYTHONPATH",
                "/data/storage/el1/bundle/entry/resources/resfile/Python/lib/python3.12:"
                "/data/storage/el1/bundle/entry/resources/resfile/Python/lib/python3.12/site-packages:"
                "/data/storage/el1/bundle/libs/arm64/Python/lib/python3.12/lib-dynload:"
                "/data/storage/el1/bundle/libs/arm64/Python/lib/python3.12/site-packages:"
                "/data/storage/el1/bundle/entry/resources/resfile/persepolis",
                1);

            LOG_D("Calling Py_Initialize()...");
            Py_Initialize();
            Py_Initialize();

            if (Py_IsInitialized()) {
                LOG_D("Python runtime initialized successfully");
                shouldFinalize = true;
            } else {
                LOG_E("Python initialization failed!");
            }
        } else {
            LOG_D("Python already initialized, skipping initialization");
        }
    }
    ~PythonRuntimeGuard() {
        if (shouldFinalize) {
            LOG_D("Cleaning up Python runtime, calling Py_Finalize()...");
            Py_Finalize();
            LOG_D("Python runtime finalized successfully");
        } else {
            LOG_D("Skipping Py_Finalize() - not initialized by this guard");
        }
    }

    bool shouldFinalize { false } ;
};

int main(int argc, char *argv[]) {
    /* Boot probe: visible side effect independent of hilog (domain-0 logs
     * are silently dropped by hilog; probe file proves main() was reached). */
    FILE *probe = fopen("/data/storage/el2/base/haps/entry/files/pyqtlauncher-booted.txt", "w");
    if (probe) { fputs("main reached\n", probe); fclose(probe); }

    LOG_I("pyqtlauncher started");

    /* Register hilog module BEFORE Py_Initialize */
    PyImport_AppendInittab("_ohos_hilog", PyInit__ohos_hilog);

    LOG_D("Opening Python script file: %s", gPythonAppPath.c_str());
    FILE *pyFile = fopen(gPythonAppPath.c_str(), "r");
    if (!pyFile) {
        LOG_E("Failed to open Python script: %s", gPythonAppPath.c_str());
        return 1;
    }

    LOG_D("Initializing Python runtime environment...");
    PythonRuntimeGuard pyRuntime;

    /* Redirect stdout/stderr to hilog */
    PyRun_SimpleString(
        "import _ohos_hilog\n"
        "class _S:\n"
        "    def __init__(s,l=1): s.l=l\n"
        "    def write(s,t): \n"
        "        if t and t.strip(): _ohos_hilog.hilog_print(t,s.l)\n"
        "    def flush(s): pass\n"
        "import sys\n"
        "sys.stdout=_S(1)\n"
        "sys.stderr=_S(4)\n"
    );

    wchar_t *pyArgv0 = Py_DecodeLocale(gPythonAppPath.c_str(), NULL);
    PySys_SetArgvEx(1, &pyArgv0, 1);

    LOG_D("Executing Python script via PyRun_FileExFlags()...");
    PyObject *mainModule = PyImport_AddModule("__main__");
    PyObject *globalDict = PyModule_GetDict(mainModule);

    /* Use globalDict as both global and local - so top-level names are visible in functions */
    PyObject *result = PyRun_FileExFlags(
        pyFile, gPythonAppPath.c_str(), Py_file_input,
        globalDict, globalDict, 1, NULL);

    int ret = 0;
    if (result == NULL) {
        PyObject *pErrType, *pErrValue, *pErrTraceback;
        PyErr_Fetch(&pErrType, &pErrValue, &pErrTraceback);
        PyErr_NormalizeException(&pErrType, &pErrValue, &pErrTraceback);

        if (pErrType != NULL && PyErr_GivenExceptionMatches(pErrType, PyExc_SystemExit)) {
            if (pErrValue != NULL && PyLong_Check(pErrValue)) {
                ret = (int)PyLong_AsLong(pErrValue);
            }
            LOG_I("Python script exited via SystemExit, code = %d", ret);
        } else {
            ret = 1;
            if (pErrValue) {
                PyObject *pStr = PyObject_Str(pErrValue);
                if (pStr) {
                    const char *msg = PyUnicode_AsUTF8(pStr);
                    if (msg) LOG_E("Python exception: %s", msg);
                    Py_DECREF(pStr);
                }
            }
            if (pErrType) {
                PyObject *pStr = PyObject_Str(pErrType);
                if (pStr) {
                    const char *msg = PyUnicode_AsUTF8(pStr);
                    if (msg) LOG_E("Python exception type: %s", msg);
                    Py_DECREF(pStr);
                }
            }
            if (pErrTraceback) {
                PyObject *pStr = PyObject_Str(pErrTraceback);
                if (pStr) {
                    const char *msg = PyUnicode_AsUTF8(pStr);
                    if (msg) LOG_E("Python traceback: %s", msg);
                    Py_DECREF(pStr);
                }
            }
        }
        Py_XDECREF(pErrType);
        Py_XDECREF(pErrValue);
        Py_XDECREF(pErrTraceback);
    } else {
        LOG_I("Python script executed successfully");
        Py_DECREF(result);
    }

    PyMem_RawFree(pyArgv0);
    return ret;
}
