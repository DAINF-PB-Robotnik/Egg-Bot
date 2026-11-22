import { useEffect, useRef } from 'react';
import { X, Egg, Check, AlertCircle } from 'lucide-react';

export function Logo({ small = false }) {
  return <div className={`brand ${small ? 'brand-small' : ''}`}><div className="brand-symbol"><Egg size={26} strokeWidth={1.6} /><span /></div><span>egg<span className="font-normal opacity-60">.studio</span></span></div>;
}
export function Button({ icon: Icon, children, variant = 'secondary', className = '', ...props }) {
  return <button className={`btn btn-${variant} ${className}`} {...props}>{Icon && <Icon size={17} strokeWidth={1.8} />}{children}</button>;
}
export function IconButton({ icon: Icon, label, active = false, ...props }) {
  return <button type="button" className={`icon-btn ${active ? 'active' : ''}`} aria-label={label} title={label} {...props}><Icon size={18} strokeWidth={1.7} /></button>;
}
export function Modal({ title, children, onClose, wide = false }) {
  const ref = useRef();
  useEffect(() => {
    const dialog = ref.current;
    dialog.showModal();
    return () => dialog.close();
  }, []);
  return <dialog ref={ref} className={`modal ${wide ? 'modal-wide' : ''}`} onCancel={onClose} onClick={e => { if (e.target === e.currentTarget) onClose(); }}>
    <div className="flex items-center justify-between gap-4 border-b border-line px-6 py-5"><h2 className="text-lg font-bold">{title}</h2><IconButton icon={X} label="Fechar janela" onClick={onClose} /></div>
    <div className="p-6">{children}</div>
  </dialog>;
}
export function Toast({ message, error, onClose }) {
  useEffect(() => { const t = setTimeout(onClose, 5500); return () => clearTimeout(t); }, [message, onClose]);
  return <div role={error ? 'alert' : 'status'} className={`toast ${error ? 'toast-error' : ''}`}>{error ? <AlertCircle size={19} /> : <Check size={19} />}<span>{message}</span><button aria-label="Fechar aviso" onClick={onClose}><X size={16} /></button></div>;
}
