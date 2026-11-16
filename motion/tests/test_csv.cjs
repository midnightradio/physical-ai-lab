const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const html = fs.readFileSync(path.join(__dirname, '../index.html'), 'utf8');
const context = { window: {}, TextDecoder, TextEncoder, Blob, Date };
vm.runInNewContext(html.match(/<script id="motion-core">([\s\S]*?)<\/script>/)[1], context);
const C = context.window.MotionCore;
let passed = 0;
function test(name, fn) { fn(); passed++; console.log(`PASS ${name}`); }
function parsed(record, format) {
  const [header, ...rows] = C.csvFor(record, format).trim().split(/\r?\n/);
  return { header: header.split(','), rows: rows.map(row => row.split(',').map(Number)) };
}
function near(actual, expected) {
  assert.ok(Math.abs(actual - expected) <= 0.0000051, `${actual} != ${expected}`);
}
function rampRecord(axes, times) {
  // Linear signals with distinct signs and units make interpolation errors visible.
  const starts = [0.1, -0.2, 1, -6, 1.5, 30];
  const slopes = [0.003, -0.002, 0.001, 0.2, -0.3, 0.4];
  return { axes, rows: times.map(t => [t, ...starts.slice(0, axes).map((v, i) => v + slopes[i] * t)]) };
}

test('3-axis CSV uses the Edge Impulse timestamp header and only accelerometer columns', () => {
  const data = parsed(rampRecord(3, [0, 20, 40]));
  assert.deepEqual(data.header, ['timestamp', 'accX', 'accY', 'accZ']);
  assert.deepEqual(data.rows.map(row => row[0]), [0, 20, 40]);
  assert.ok(data.rows.every(row => row.length === 4));
});

test('6-axis CSV keeps g and degrees/second values without unit conversion or pose columns', () => {
  const data = parsed(rampRecord(6, [0, 20, 40]));
  assert.deepEqual(data.header, ['timestamp', 'accX', 'accY', 'accZ', 'gyrX', 'gyrY', 'gyrZ']);
  assert.deepEqual(data.rows[0], [0, 0.1, -0.2, 1, -6, 1.5, 30]);
  assert.ok(data.rows.every(row => row.length === 7));
});

for (const axes of [3, 6]) {
  test(`${axes}-axis jitter is interpolated onto a strict 20ms grid without changing captured rows`, () => {
    const record = rampRecord(axes, [0, 21, 41, 62, 82]);
    const before = JSON.stringify(record);
    const data = parsed(record);
    const expected = rampRecord(axes, [0, 20, 40, 60, 80]);
    assert.deepEqual(data.rows.map(row => row[0]), [0, 20, 40, 60, 80]);
    data.rows.forEach((row, i) => row.forEach((value, j) => near(value, expected.rows[i][j])));
    assert.equal(JSON.stringify(record), before);
  });
}

test('interpolation uses both surrounding observations and does not round to a nearby sample', () => {
  const data = parsed({ axes: 3, rows: [[0, 0, 4, -1], [21, 42, -17, 1.1], [41, 2, 3, 3.1]] });
  assert.deepEqual(data.rows, [[0, 0, 4, -1], [20, 40, -16, 1], [40, 4, 2, 3]]);
});

test('the last observed timestamp is not extrapolated to the requested capture duration', () => {
  const data = parsed({ ...rampRecord(6, [0, 21, 41, 62]), seconds: 10 });
  assert.deepEqual(data.rows.map(row => row[0]), [0, 20, 40, 60]);
  assert.equal(data.rows.at(-1)[0], 60);
});

test('the exact 40ms gap boundary is accepted and interpolated', () => {
  assert.deepEqual(parsed(rampRecord(3, [0, 40])).rows.map(row => row[0]), [0, 20, 40]);
});

test('a gap above 40ms blocks Edge Impulse export while raw export preserves actual observations', () => {
  const record = { axes: 3, rows: [[0, 0.1, -0.2, 1], [20, 0.3, 0.4, 0.9], [61, 0.5, 0.6, 0.8]] };
  assert.throws(() => C.csvFor(record, 'edgeimpulse'));
  assert.equal(C.csvFor(record, 'raw'), 'timestamp,accX,accY,accZ\r\n0,0.10000,-0.20000,1.00000\r\n20,0.30000,0.40000,0.90000\r\n61,0.50000,0.60000,0.80000\r\n');
});

test('raw 6-axis export retains irregular timestamps and original measurement precision policy', () => {
  const record = { axes: 6, rows: [[0, 0.123456, -0.2, 1, -6, 1.5, 30], [21, 0.3, 0.4, 0.9, -5.9, 1.4, 29]] };
  assert.equal(C.csvFor(record, 'raw'), 'timestamp,accX,accY,accZ,gyrX,gyrY,gyrZ\r\n0,0.12346,-0.20000,1.00000,-6.00000,1.50000,30.00000\r\n21,0.30000,0.40000,0.90000,-5.90000,1.40000,29.00000\r\n');
});

test('too few observations or fewer than two 20ms grid positions are rejected', () => {
  for (const times of [[], [0], [0, 19]]) assert.throws(() => C.csvFor(rampRecord(3, times)));
});

test('nonzero origins, negative, repeated, or decreasing timestamps are rejected', () => {
  for (const times of [[1, 21], [-1, 20], [0, 20, 20], [0, 20, 19]]) {
    assert.throws(() => C.csvFor(rampRecord(3, times)));
  }
});

test('invalid sensor axes and missing, extra, or non-finite measurements are rejected', () => {
  for (const axes of [0, 4, 9]) assert.throws(() => C.csvFor({ axes, rows: [[0, 0, 0, 1], [20, 0, 0, 1]] }));
  for (const bad of [NaN, Infinity, -Infinity, undefined, '1']) {
    const record = rampRecord(3, [0, 20]); record.rows[1][2] = bad;
    assert.throws(() => C.csvFor(record));
  }
  for (const row of [[20, 0, 1], [20, 0, 0, 1, 2]]) {
    assert.throws(() => C.csvFor({ axes: 3, rows: [[0, 0, 0, 1], row] }));
  }
});

test('unknown export formats fail instead of silently changing the data', () => {
  assert.throws(() => C.csvFor(rampRecord(3, [0, 20]), 'typo'));
});

console.log(`${passed} CSV tests passed.`);
