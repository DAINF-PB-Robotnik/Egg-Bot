import { Play, Pause, Square, Trash2, Download, FileCheck2, Clock3, Layers3, CheckCircle2, Printer, CircleHelp } from 'lucide-react';
import { Button } from './ui';
import EggPreview from './EggPreview';
import { download, formatTime } from '../lib/drawing';

const labels = { idle: 'Tudo pronto para começar', running: 'Seu desenho está ganhando forma', paused: 'Uma pausa no traço', stopping: 'Finalizando o trabalho', done: 'Mais uma ideia ganhou forma', error: 'Vamos conferir a conexão' };
export default function PrintPanel({ machine, act, confirm, goCreate }) {
  const { status, printing, unavailable, remote, request } = machine;
  const job = status?.job, state = status?.print || { state: 'idle', done: 0, total: 0 };
  const percent = state.total ? Math.min(100, Math.round(state.done / state.total * 100)) : 0;
  const blocked = unavailable || status?.uno.busy || status?.cfg_busy;
  return <div className="print-layout animate-in">
    <div className="space-y-5">
      <section className="panel p-7 sm:p-9">
        <div className="flex items-center justify-between"><span className="section-kicker">NA FILA DO EGGBOT</span><span className={`state-pill ${state.state === 'running' ? 'state-running' : ''}`}>{({ idle: 'Aguardando', running: 'Imprimindo', paused: 'Pausado', stopping: 'Finalizando', done: 'Concluído', error: 'Erro' })[state.state]}</span></div>
        <div className="print-illustration">{state.state === 'done' ? <CheckCircle2 size={42} strokeWidth={1.2} /> : <Printer size={42} strokeWidth={1.2} />}</div>
        <h2 className="text-center text-2xl font-semibold tracking-tight">{job?.present ? labels[state.state] : 'Uma tela em branco. Muitas possibilidades.'}</h2>
        <p className="mx-auto mt-3 max-w-md text-center text-sm leading-6 text-muted">{job?.present ? 'Confira o ovo e a posição da caneta. Quando estiver tudo certo, é só dar o primeiro passo.' : 'Crie um desenho no estúdio e envie para vê-lo ganhar vida aqui.'}</p>
        {job?.present ? <>
          <div className="job-file"><span className="file-icon"><FileCheck2 size={23} /></span><div className="min-w-0 flex-1"><strong className="block truncate text-sm">{job.name}.egg</strong><span className="text-xs text-muted">{(job.bytes / 1024).toFixed(1)} KB · {job.points} pontos · arquivo verificado</span></div><button className="icon-btn" aria-label="Baixar trabalho do EggBot" disabled={!remote} onClick={() => download(remote, `${job.name}.egg`)}><Download size={18} /></button></div>
          <div className="my-7"><div className="mb-3 flex items-center justify-between text-sm"><span className="text-muted">Progresso dos comandos</span><strong>{percent}%</strong></div><div className="progress-track"><div style={{ width: `${percent}%` }} /></div><p className="mt-2 text-xs text-muted">{state.done} de {state.total || job.ebb_cmds} comandos aceitos</p></div>
          <div className="grid grid-cols-2 gap-4 border-y border-line py-5"><div className="flex items-center gap-3"><Clock3 size={19} className="text-muted" /><div><p className="text-xs text-muted">Tempo decorrido</p><strong className="text-sm">{formatTime(state.elapsed_s)}</strong></div></div><div className="flex items-center gap-3"><Layers3 size={19} className="text-muted" /><div><p className="text-xs text-muted">{printing ? 'Restante estimado' : 'Duração estimada'}</p><strong className="text-sm">{formatTime(printing ? state.remaining_s : job.est_s)}</strong></div></div></div>
          <div className="mt-6 flex flex-wrap gap-3">
            {!printing && <Button variant="primary" icon={Play} disabled={blocked} onClick={() => act(() => request('/print/start'), 'Trabalho iniciado.')}>Iniciar impressão</Button>}
            {state.state === 'running' && <Button variant="primary" icon={Pause} disabled={unavailable} onClick={() => act(() => request('/print/pause'), 'Pausa solicitada.')}>Pausar</Button>}
            {state.state === 'paused' && <Button variant="primary" icon={Play} disabled={unavailable} onClick={() => act(() => request('/print/resume'), 'Impressão retomada.')}>Continuar</Button>}
            {printing ? <Button icon={Square} disabled={unavailable || state.state === 'stopping'} onClick={() => confirm('Cancelar impressão?', 'O EggBot receberá a solicitação de levantar a caneta e retornar ao início.', () => act(() => request('/print/cancel'), 'Cancelamento solicitado.'))}>Cancelar</Button> : <Button icon={Trash2} disabled={unavailable} onClick={() => confirm('Apagar trabalho?', 'O arquivo armazenado no EggBot será removido. Seu desenho no estúdio será preservado.', () => act(() => request('/job/delete'), 'Trabalho removido.'))}>Apagar arquivo</Button>}
          </div>
        </> : <div className="mt-8 text-center"><Button variant="primary" icon={Play} onClick={goCreate}>Abrir estúdio</Button></div>}
        {state.message && <div role="status" className={`notice mt-5 ${state.state === 'error' ? 'notice-error' : ''}`}>{state.message}</div>}
      </section>
      <div className="flex gap-3 px-2 text-sm leading-6 text-muted"><CircleHelp className="mt-1 shrink-0" size={18} /><p>Acompanhe a máquina durante a impressão. O progresso indica comandos aceitos; a posição dos motores é estimada.</p></div>
    </div>
    <EggPreview file={remote} source="Arquivo armazenado no EggBot · projeção ilustrativa" large />
  </div>;
}
