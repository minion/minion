// Cargo runner: verify the actual module's memory before executing the test.
import { readFileSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import assert from 'node:assert/strict';
const [file, ...args] = process.argv.slice(2);
const bytes = readFileSync(file.replace(/\.js$/, '.wasm'));
let pos = 8;
function uleb() {
  let value = 0, shift = 0, byte;
  do {
    byte = bytes[pos++];
    assert.ok(byte !== undefined && shift < 35, 'invalid Wasm LEB');
    value += (byte & 127) * 2 ** shift;
    shift += 7;
  } while (byte & 128);
  return value;
}
let memories = 0;
while (pos < bytes.length) {
  const section = bytes[pos++];
  const size = uleb(), end = pos + size;
  if (section === 5) {
    memories = uleb();
    assert.equal(memories, 1);
    const flags = uleb();
    assert.equal(flags & 2, 0, 'Minion must use unshared memory');
  }
  pos = end;
}
assert.equal(memories, 1, 'expected one defined Wasm memory');
const result = spawnSync(process.execPath, [file, ...args], { stdio: 'inherit' });
if (result.error) throw result.error;
assert.equal(result.signal, null, 'Node terminated by signal');
process.exit(result.status ?? 1);
