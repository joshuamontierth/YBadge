const express = require('express');
const http = require('http');
const bodyParser = require('body-parser');
const socketIo = require('socket.io');

const app = express();
const server = http.createServer(app);
const io = socketIo(server);  // Attach socket.io to the server

app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());

// Serve your index.html file here
app.get('/', (req, res) => {
    res.sendFile(__dirname + '/index.html');
});

// Endpoint to receive data from ESP32 for a button
app.post('/button', (req, res) => {
    console.log('Button data received:', req.body.data);
    io.emit('updateButtonData', req.body.data);  // Emit button data to all connected clients
    res.status(200).send('Button Data Received');
});

// Endpoint to receive data from ESP32 for a switch
app.post('/switch', (req, res) => {
    console.log('Switch data received:', req.body.data);
    io.emit('updateSwitchData', req.body.data);  // Emit switch data to all connected clients
    res.status(200).send('Switch Data Received');
});

app.post('/knob', (req, res) => {
    console.log('Knob data received:', req.body.data);
    io.emit('updateKnobData', req.body.data);  // Emit knob data to all connected clients
    res.status(200).send('Knob Data Received');
});

io.on('connection', (socket) => {
    console.log('A user connected');
});

const port = 5000;
server.listen(port, () => {
    console.log(`Server running on http://localhost:${port}`);
});
