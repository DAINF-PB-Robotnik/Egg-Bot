import { useCallback, useEffect, useRef, useState } from 'react';
import { createDemoTransport } from '../lib/demo';

export const IS_DEMO = import.meta.env.VITE_DEMO !== 'false';
const base = (import.meta.env.VITE_API_BASE ?? '/api').replace(/\/$/, '');

export function useMachine() {
  const transport = useRef(null), mounted = useRef(false), locked = useRef(false);
  const [status, setStatus] = useState(null), [online, setOnline] = useState(false), [busy, setBusy] = useState(false), [remote, setRemote] = useState('');
  const signature = useRef('');
  const refresh = useCallback(async () => {
    if (!transport.current) return;
    try {
      const response = await transport.current.request('/status', { cache: 'no-store' });
      if (!response.ok) throw new Error('Não foi possível consultar o EggBot.');
      const data = await response.json();
      if (!mounted.current) return;
      setStatus(data); setOnline(true);
      const sig = data.job.present ? data.job.crc + data.job.bytes : '';
      if (sig !== signature.current) {
        if (!sig) { signature.current = ''; setRemote(''); }
        else {
          const file = await transport.current.request('/job/raw', { cache: 'no-store' });
          if (!file.ok) throw new Error('Não foi possível ler o trabalho.');
          const text = await file.text();
          if (mounted.current) { signature.current = sig; setRemote(text); }
        }
      }
    } catch { if (mounted.current) setOnline(false); }
  }, []);
  useEffect(() => {
    mounted.current = true;
    transport.current = IS_DEMO ? createDemoTransport() : {
      request: (path, options) => fetch(base + path, { ...options, signal: AbortSignal.timeout(8000) }), dispose() {},
    };
    let timer, stopped = false;
    const poll = async () => { await refresh(); if (!stopped) timer = setTimeout(poll, 800); };
    poll();
    return () => { stopped = true; mounted.current = false; clearTimeout(timer); transport.current?.dispose(); transport.current = null; };
  }, [refresh]);

  const request = useCallback(async (path, options = {}) => {
    if (!transport.current || locked.current) throw new Error('Aguarde a operação atual.');
    locked.current = true; setBusy(true);
    try {
      const response = await transport.current.request(path, { method: 'POST', ...options });
      const data = await response.json();
      if (!response.ok || !data.ok) throw new Error(data.message || 'Não foi possível concluir a operação.');
      await refresh();
      return data;
    } finally { locked.current = false; if (mounted.current) setBusy(false); }
  }, [refresh]);
  const upload = useCallback(file => {
    const data = new FormData(); data.append('file', new Blob([file], { type: 'text/plain' }), 'job.txt');
    return request('/job', { body: data });
  }, [request]);
  const command = useCallback(c => request(`/cmd?c=${encodeURIComponent(c)}`), [request]);
  const printing = ['running', 'paused', 'stopping'].includes(status?.print.state);
  const unavailable = busy || !online || !status;
  return { status, online, busy, remote, printing, unavailable, request, upload, command, refresh };
}
