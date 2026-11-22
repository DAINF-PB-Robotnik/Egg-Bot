import { defineConfig, loadEnv } from 'vite';
import react from '@vitejs/plugin-react';
import tailwindcss from '@tailwindcss/vite';

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '');
  return {
    plugins: [react(), tailwindcss()],
    server: {
      host: '127.0.0.1',
      proxy: { '/api': { target: env.EGGBOT_TARGET || 'http://192.168.4.1', changeOrigin: true, rewrite: path => path.replace(/^\/api/, '') } },
    },
  };
});
