if(!self.define){let e,s={};const i=(i,n)=>(i=new URL(i+".js",n).href,s[i]||new Promise((s=>{if("document"in self){const e=document.createElement("script");e.src=i,e.onload=s,document.head.appendChild(e)}else e=i,importScripts(i),s()})).then((()=>{let e=s[i];if(!e)throw new Error(`Module ${i} didn’t register its module`);return e})));self.define=(n,r)=>{const t=e||("document"in self?document.currentScript.src:"")||location.href;if(s[t])return;let o={};const l=e=>i(e,t),c={module:{uri:t},exports:o,require:l};s[t]=Promise.all(n.map((e=>c[e]||l(e)))).then((e=>(r(...e),o)))}}define(["./workbox-3e911b1d"],(function(e){"use strict";self.skipWaiting(),e.clientsClaim(),e.precacheAndRoute([{url:"assets/index-bVsUjWfj.css",revision:null},{url:"assets/index-cT_sXTxz.js",revision:null},{url:"index.html",revision:"a16291b7e68b6bcac5ae37a65a9e77d0"},{url:"registerSW.js",revision:"402b66900e731ca748771b6fc5e7a068"},{url:"./pwa-512x512.png",revision:"1ed52252f0c1bf82df192ea92611584c"},{url:"manifest.webmanifest",revision:"bc888215ff5e53878e3d575ee517ac52"}],{}),e.cleanupOutdatedCaches(),e.registerRoute(new e.NavigationRoute(e.createHandlerBoundToURL("index.html")))}));