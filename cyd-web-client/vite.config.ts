import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react-swc'
import basicSsl from '@vitejs/plugin-basic-ssl'
import { VitePWA } from 'vite-plugin-pwa'
//
const manifest = {
  theme_color: "#000000",
  icons: [
    {
      src: "/pwa-512x512.png",
      sizes: "512x512",
      type: "image/png",
      purpose: "any maskable"
    }
  ]
}
//
export default defineConfig({
  plugins: [react(), basicSsl(), VitePWA({
    registerType: 'autoUpdate',

    manifest: manifest,
    devOptions: {
      enabled: true,

    }
  })],
})
