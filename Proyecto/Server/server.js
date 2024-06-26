const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const cors = require('cors');

const app = express();
app.use(cors());
app.use(express.json());

const server = http.createServer(app);
const io = socketIo(server, {
  cors: {
    origin: "http://localhost:3000",
    methods: ["GET", "POST"]
  }
});

const PORT = process.env.PORT || 3001;

const memoryData = {
  pieChartData: [
    { name: 'Chrome', memory: 750 },
    { name: 'Node.js', memory: 500 },
    { name: 'Spotify', memory: 250 },
    { name: 'Slack', memory: 150 },
    { name: 'Visual Studio Code', memory: 100 },
    { name: 'Otros', memory: 250 },
  ],
  processes: [
    { pid: 2406, name: 'Chrome', memory: 750, percentage: 0 },
    { pid: 2407, name: 'Node.js', memory: 500, percentage: 0 },
    { pid: 2408, name: 'Spotify', memory: 250, percentage: 0 },
  ],
  requests: [
    { pid: 2406, call: 'mmap', size: '20MB', date: '2024/06/24 10:00:00' },
    { pid: 2407, call: 'munmap', size: '15MB', date: '2024/06/24 10:01:00' },
    { pid: 2408, call: 'mmap', size: '30MB', date: '2024/06/24 10:02:00' },
  ],
};

const calculatePercentages = (data) => {
  const totalMemory = data.pieChartData.reduce((sum, process) => sum + process.memory, 0);
  data.pieChartData.forEach((process) => {
    process.percentage = ((process.memory / totalMemory) * 100).toFixed(2);
  });
  data.processes.forEach((process) => {
    process.percentage = ((process.memory / totalMemory) * 100).toFixed(2);
  });
};

calculatePercentages(memoryData);

app.get('/api/memory-data', (req, res) => {
  res.json(memoryData);
});

io.on('connection', (socket) => {
  console.log('New client connected');

  const sendData = () => {
    socket.emit('memoryData', memoryData);
  };

  const interval = setInterval(sendData, 5000);

  socket.on('disconnect', () => {
    clearInterval(interval);
    console.log('Client disconnected');
  });
});

server.listen(PORT, () => console.log(`Listening on port ${PORT}`));
