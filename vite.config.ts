import react from '@vitejs/plugin-react';
import { defineConfig } from 'vitest/config';
import { VitePWA } from 'vite-plugin-pwa';

export default defineConfig({
  base: './',
  plugins: [
    react(),
    VitePWA({
      registerType: 'autoUpdate',
      includeAssets: ['icons/*.png', 'icons/*.svg'],
      workbox: {
        globPatterns: ['**/*.{js,css,html,woff2,png,svg,json,mp3}'],
        maximumFileSizeToCacheInBytes: 5_000_000,
      },
      manifest: {
        name: 'קשת הסולמות',
        short_name: 'קשת',
        description: 'לימוד סולמות ובניית שמיעה עם מטרונום וצבעים',
        lang: 'he',
        dir: 'rtl',
        start_url: '.',
        display: 'standalone',
        background_color: '#141120',
        theme_color: '#141120',
        icons: [
          { src: 'icons/icon-192.png', sizes: '192x192', type: 'image/png' },
          { src: 'icons/icon-512.png', sizes: '512x512', type: 'image/png' },
          { src: 'icons/icon-512.png', sizes: '512x512', type: 'image/png', purpose: 'maskable' },
        ],
      },
    }),
  ],
  test: { environment: 'node' },
});
