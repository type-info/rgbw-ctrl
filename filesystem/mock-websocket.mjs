import {WebSocketServer} from "ws";

// Create a WebSocket server on port 8080
const wss = new WebSocketServer({port: 80});

console.log('WebSocket server is running on ws://localhost:80');

// Connection event handler
wss.on('connection', (ws) => {
    console.log('New client connected');

    // Message event handler
    ws.on('message', (message) => {
        console.log(`Received: ${message}`);
        // Echo the message back to the client
    });

    // Close event handler
    ws.on('close', () => {
        console.log('Client disconnected');
    });
});