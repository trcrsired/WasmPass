// Increment this version when you change files
const CACHE_VERSION = "v1";
const CACHE_NAME = `genpass-cache-${CACHE_VERSION}`;

const ASSETS = [
  "/",                   // root (allows index resolution)
  "/app.js",             // WASM loader
  "/sw-register.js",     // service worker registration
  "/genpass.wasm",       // WebAssembly binary
  "/manifest.webmanifest", // PWA manifest
  "/style.css",          // styles
  "/icon.webp"           // app icon
];

// Install: pre-cache essential assets
self.addEventListener("install", event => {
  event.waitUntil(
    caches.open(CACHE_NAME).then(cache => cache.addAll(ASSETS))
  );
});

// Activate: clean up old caches
self.addEventListener("activate", event => {
  event.waitUntil(
    caches.keys().then(keys =>
      Promise.all(
        keys.filter(key => key !== CACHE_NAME)
            .map(key => caches.delete(key))
      )
    )
  );
});

// Fetch: cache-first for static assets, network-first for WASM
self.addEventListener("fetch", event => {
  const { request } = event;
  event.respondWith(
    caches.match(request).then(resp => resp || fetch(request))
  );
});
