// Minimal browserless test helper for POST /run using Node's built-in FormData + fetch.
// No dependencies.

const fs = require('fs');

(async () => {
  const filePath = process.argv[2] || '../sample_test.js';
  const api = process.argv[3] || 'http://localhost:8081/run';

  const form = new FormData();
  const fileBuf = fs.readFileSync(filePath);

  // Node's fetch/FormData in recent Node versions supports Blob/Buffer.
  // We'll send it as raw bytes.
  form.append('file', new Blob([fileBuf], { type: 'application/javascript' }), 'sample_test.js');

  const res = await fetch(api, { method: 'POST', body: form });
  const text = await res.text();
  console.log('HTTP', res.status);
  console.log(text);
})();

