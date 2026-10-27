import { kamelLoad } from 'libkamel_napi.so';
import { worker, MessageEvents } from '@kit.ArkTS';

// Worker-side entry. KamelLoad blocks on network IO for possibly seconds,
// so it must run off the UI thread. Inside a worker script the canonical
// handle is `worker.workerPort` (ThreadWorkerGlobalScope) — the ThreadWorker
// class is main-thread-only here, its constructor requires a scriptURL.
const port = worker.workerPort;

port.onmessage = (e: MessageEvents): void => {
  const request: string = e.data as string;
  const raw: string = kamelLoad(request);
  port.postMessage(raw);
};
