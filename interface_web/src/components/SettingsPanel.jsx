import { useEffect, useState } from 'react';
import { SlidersHorizontal, PenTool, MoveVertical, Gauge, Save, RotateCcw, Crosshair, Terminal, Wifi, ChevronDown } from 'lucide-react';
import { Button } from './ui';
import { DEFAULT_CONFIG } from '../lib/drawing';
import { IS_DEMO } from '../hooks/useMachine';

function NumberField({ label, value, onChange, min, max, suffix = '', disabled }) {
  return <label className="number-field"><span>{label}</span><div><input aria-label={label} type="number" value={value} min={min} max={max} required disabled={disabled} onChange={e => onChange(e.target.value === '' ? '' : Number(e.target.value))} /><span>{suffix}</span></div></label>;
}
export default function SettingsPanel({ machine, act, notify }) {
  const { status, printing, unavailable, command, request } = machine;
  const [form, setForm] = useState(status?.cfg || DEFAULT_CONFIG), [dirty, setDirty] = useState(false), [consoleText, setConsoleText] = useState('');
  useEffect(() => { if (!dirty && status?.cfg) setForm(status.cfg); }, [status?.cfg, dirty]);
  const set = (key, value) => { setForm(f => ({ ...f, [key]: value })); setDirty(true); };
  const blocked = unavailable || printing, manualBlocked = blocked || status?.uno.busy || status?.cfg_busy;
  const arm = status?.arm_pos || 0;
  async function save(event) {
    event.preventDefault();
    await act(async () => {
      const result = await request(`/config?${new URLSearchParams(form)}`);
      setDirty(false);
      notify(result.job_dropped ? 'Configuração salva. O trabalho fora dos novos limites foi removido.' : 'Configuração salva.');
    });
  }
  return <div className="settings-layout animate-in">
    <form className="space-y-5" onSubmit={save}>
      <section className="panel p-6 sm:p-7"><div className="settings-heading"><span><PenTool size={21} /></span><div><h2>O toque da caneta</h2><p>Ajuste as posições e o tempo do servo.</p></div></div>
        {[['pen_up', 'Caneta levantada', 'up'], ['pen_down', 'Caneta abaixada', 'down']].map(([key, label, pos]) => <div className="servo-field" key={key}><label htmlFor={key}>{label}<strong>{form[key]}%</strong></label><div className="flex items-center gap-4"><input id={key} type="range" min="0" max="100" value={form[key]} disabled={blocked} onChange={e => set(key, +e.target.value)} /><Button type="button" disabled={manualBlocked} onClick={() => act(() => request(`/pen/test?pos=${pos}&pct=${form[key]}`), 'Teste solicitado. A posição foi enviada ao Arduino.')}>Testar</Button></div></div>)}
        <NumberField label="Tempo de levantar / baixar" value={form.pen_ms} onChange={v => set('pen_ms', v)} min={100} max={2000} suffix="ms" disabled={blocked} />
        <p className="mt-4 text-xs leading-5 text-muted">Testar aplica a posição no Arduino. Descartar restaura apenas os campos da tela; não desfaz o teste no servo.</p>
      </section>
      <section className="panel p-6 sm:p-7"><div className="settings-heading"><span><MoveVertical size={21} /></span><div><h2>Encontre o centro</h2><p>Posicione o braço e delimite a área de desenho.</p></div></div>
        <div className="arm-position"><span>Posição estimada do braço</span><strong>{arm}<small> passos</small></strong></div>
        <div className="my-4 grid grid-cols-4 gap-2">{[-50, -10, 10, 50].map(n => <Button key={n} type="button" disabled={manualBlocked} onClick={() => act(() => command(`SM,${Math.abs(n) === 50 ? 250 : 100},${n},0`))}>{n > 0 ? '+' : '−'} {Math.abs(n)}</Button>)}</div>
        <div className="mb-5 flex flex-wrap gap-2"><Button type="button" icon={Crosshair} disabled={manualBlocked || status?.uno.moving_ms > 0} onClick={() => act(() => request('/arm/zero'), 'Centro definido.')}>Definir centro aqui</Button><Button type="button" icon={RotateCcw} disabled={manualBlocked || !arm} onClick={() => { const step = Math.max(-1600, Math.min(1600, -arm)); act(() => command(`SM,${Math.max(100, Math.ceil(Math.abs(step) * 1000 / 400))},${step},0`)); }}>Voltar ao centro</Button></div>
        <NumberField label="Limite inferior" value={form.arm_min} onChange={v => set('arm_min', v)} min={-1600} max={-10} suffix="passos" disabled={blocked} />
        <NumberField label="Limite superior" value={form.arm_max} onChange={v => set('arm_max', v)} min={10} max={1600} suffix="passos" disabled={blocked} />
        <div className="mt-3 flex flex-wrap gap-2"><Button type="button" disabled={blocked || arm > -10 || arm < -1600} onClick={() => set('arm_min', arm)}>Marcar inferior aqui</Button><Button type="button" disabled={blocked || arm < 10 || arm > 1600} onClick={() => set('arm_max', arm)}>Marcar superior aqui</Button></div>
        <p className="mt-4 text-xs leading-5 text-muted">Ao desligar os motores ou reposicionar o braço, defina o centro novamente.</p>
      </section>
      <section className="panel p-6 sm:p-7"><div className="settings-heading"><span><Gauge size={21} /></span><div><h2>O ritmo do desenho</h2><p>Velocidades para cada momento do percurso.</p></div></div><NumberField label="Desenhando" value={form.speed_down} onChange={v => set('speed_down', v)} min={50} max={1000} suffix="passos/s" disabled={blocked} /><NumberField label="Deslocando" value={form.speed_up} onChange={v => set('speed_up', v)} min={50} max={1000} suffix="passos/s" disabled={blocked} /></section>
      <div className="save-bar"><span>{dirty ? 'Alterações não salvas' : 'Configuração salva'}</span><div className="flex gap-2"><Button type="button" disabled={!dirty} onClick={() => { setForm(status?.cfg || DEFAULT_CONFIG); setDirty(false); }}>Descartar</Button><Button type="submit" variant="primary" icon={Save} disabled={!dirty || blocked}>Salvar</Button></div></div>
    </form>
    <aside className="space-y-5"><section className="panel p-6"><div className="settings-heading"><span><Wifi size={21} /></span><div><h2>Seu EggBot</h2><p>{IS_DEMO ? 'Ambiente de demonstração' : 'Conexão com a máquina'}</p></div></div><dl className="detail-list"><div><dt>Rede</dt><dd>{status?.ssid || '—'}</dd></div><div><dt>Arduino</dt><dd>{status?.uno.state === 'ok' ? 'Respondendo' : 'Aguardando'}</dd></div><div><dt>Comunicação</dt><dd>9600 baud</dd></div><div><dt>Erros registrados</dt><dd>{status?.uno.errors || 0}</dd></div></dl><Button type="button" className="mt-5 w-full" icon={Wifi} disabled={manualBlocked} onClick={() => act(() => command('v'), 'Verificação solicitada.')}>Verificar conexão</Button></section>
      <section className="panel p-6"><div className="settings-heading"><span><SlidersHorizontal size={21} /></span><div><h2>Controle manual</h2><p>Ajustes de bancada.</p></div></div><div className="manual-grid">{[['Ligar motores', 'EM,1'], ['Desligar motores', 'EM,0'], ['Levantar caneta', 'SP,0'], ['Baixar caneta', 'SP,1'], ['Girar −200', 'SM,1000,0,-200'], ['Girar +200', 'SM,1000,0,200']].map(([label, cmd]) => <Button key={cmd} disabled={manualBlocked} onClick={() => act(() => command(cmd))}>{label}</Button>)}</div></section>
      <details className="panel overflow-hidden"><summary className="flex cursor-pointer items-center justify-between p-5 text-sm font-semibold"><span className="flex items-center gap-2"><Terminal size={17} /> Console de diagnóstico</span><ChevronDown size={16} /></summary><div className="px-5 pb-5"><form className="flex gap-2" onSubmit={e => { e.preventDefault(); const c = consoleText.trim(); if (c) act(async () => { await command(c); setConsoleText(''); }); }}><input className="text-input min-w-0" aria-label="Comando EBB" maxLength={32} placeholder="Ex.: QP" value={consoleText} onChange={e => setConsoleText(e.target.value)} /><Button disabled={manualBlocked || !consoleText.trim()}>Enviar</Button></form><div className="console-log mt-4" role="log" aria-label="Log de comunicação">{status?.log?.length ? [...status.log].reverse().map((entry, i) => <p key={`${entry.t}-${i}`}><span>{entry.dir === '>' ? 'ENV' : entry.dir === '<' ? 'REC' : 'ERR'}</span> {entry.txt}</p>) : <p>Aguardando mensagens…</p>}</div></div></details>
    </aside>
  </div>;
}
