// websocket server for evaluating javascript expression remotely

var http = require("http");
var assert = require('assert');

var srv = http.createServer(function(req, res) {
  res.writeHead(200, {'Content-Type': 'text/plain'});
  res.end('simple websocket server for calculator');
});

function base64sha1(chunk) {
  var sha1 = require('crypto').createHash('sha1');
  sha1.update(chunk);
  return sha1.digest('base64');
};

function handshake(headers, sock) {
  var websock_key = headers['sec-websocket-key'];
  websock_key +=  '258EAFA5-E914-47DA-95CA-C5AB0DC85B11';
  sock.write('HTTP/1.1 101 Switching Protocols\r\n');
  sock.write("Upgrade: websocket\r\n");
  sock.write("Connection: Upgrade\r\n");
  sock.write("Sec-WebSocket-Accept: "+base64sha1(websock_key)+"\r\n\r\n");
}


var WebSocketActions = [];

function encode_text_frame(text) {
  if (text.length > 125) {
    text = text.slice(0, 125);
  }
  var buffer = new Buffer([0x81, text.length]);
  text = new Buffer(text);
  return Buffer.concat([buffer, text]);
}

// utility functions for reading bytes
// first byte
function make_fin_reader(sock) {
  return function fin_reader(byte) {
    var state = {};
    state.sock = sock;
    state.opcode = byte & 0x0f;
    state.fin = !!(byte & 0x80);
    return make_payload_reader(state);
  }
}


function make_payload_reader(state) {
  return function payload_reader(byte) {
    state.mask = byte & 0x80;
    byte &= 0x7f;

    if (byte >= 0 && byte <= 125) {
      state.length = byte;
      return make_body_reader(state);
    } else if (byte === 126) {
      return make_payload_reader_2(state);
    } else if (byte === 127) {
      return make_payload_reader_8(state);
    }
  }
}

function make_payload_reader_2(state) {
  var buffer = [];
  function payload_reader_2(byte) {
    buffer.push(byte);
    if (buffer.length === 2) {
      state.length = network_byte_order(buffer);
      return make_body_reader(state);
    } else {
      return payload_reader_2;
    }
  }
  return payload_reader_2;
}

function network_byte_order(buf) {
  var order;
  var sum = 0;
  for (var i=0; i < buf.length; i++) {
    order = buf.length - i - 1;
    if (buf[i] !== 0) {
      sum += buf[i] * Math.pow(2, order*8)
    }
  }
  return sum;
}

function make_payload_reader_8(state) {
  var buffer = [];
  function payload_reader_8(byte) {
    buffer.push(byte);
    if (buffer.length === 8) {
      state.length = network_byte_order(buffer);
      return make_body_reader(state);
    } else {
      return payload_reader_8;
    }
  }
  return payload_reader_8;
}

function make_body_reader(state) {
  if (state.mask) {
    return make_masked_body_reader(state);
  } else {
    return make_unmasked_body_reader(state);
  }
}

function unmasked_body_reader(state) {
  return function body_reader(byte) {
    return body_reader;
  }
}

function make_masked_body_reader(state) {
  var masks = [];
  var buffer = [];
  function maskkey_reader(byte) {
    masks.push(byte);
    if (masks.length === 4) {
      state.masks = masks;
      return body_reader;
    } else {
      return maskkey_reader;
    }
  }

  function body_reader(byte) {
    var text;

    var offset = buffer.length;
    byte ^= state.masks[offset % 4];
    buffer.push(byte);
    if (buffer.length === state.length) {
      text = (new Buffer(buffer)).toString('utf8');
      console.log(text);

      try {
        result = eval(text);
      } catch(e) {
        result = null;
      }
      state.sock.write(encode_text_frame(String(result)))

      return make_fin_reader(state.sock);
    } else {
      return body_reader;
    }
  }
  return maskkey_reader;
}


function makereader(sock) {

  var action = make_fin_reader(sock);
  return function reader(byte) {
    action = action(byte);
  }
}

srv.on('upgrade', function(req, sock, head) {
  var reader = makereader(sock);

  handshake(req.headers, sock);
  sock.on('data', function(chunk) {
    for (var i=0; i < chunk.length; i++) {
      reader(chunk[i]);
    }
  });
});

srv.listen(8181, '127.0.0.1');
