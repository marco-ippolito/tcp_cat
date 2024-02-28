const test = require('brittle')
const tcpCat = require('.')

test('basic url fetch', async (t) => {
  const response = await tcpCat('1.1.1.1', 80, 'GET / HTTP/1.1\r\n\r\nHost: cloudflare.com\r\n\r\n')
  t.ok(response.includes('HTTP/1.1 400 Bad Request'))
  t.ok(response.includes('Server: cloudflare'))
})

test('throws on invalid values', async (t) => {
  t.plan(3)
  // invalid host
  await t.exception(tcpCat(100, 80, 'GET / HTTP/1.1\r\n\r\nHost: cloudflare.com\r\n\r\n'))
  // invalid port
  await t.exception(tcpCat('1.1.1.1', 'foo', 'GET / HTTP/1.1\r\n\r\nHost: cloudflare.com\r\n\r\n'))
  // missing body
  await t.exception(tcpCat('1.1.1.1', 'foo'))
})
