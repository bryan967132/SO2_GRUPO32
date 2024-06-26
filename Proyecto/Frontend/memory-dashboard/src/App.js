import React, { useEffect, useState } from 'react';
import io from 'socket.io-client';
import MemoryPieChart from './components/MemoryPieChart';
import ProcessTable from './components/ProcessTable';
import RequestTable from './components/RequestTable';
import './App.css';

const socket = io('http://localhost:3001'); // Cambia la URL según tu API

const App = () => {
  const [memoryData, setMemoryData] = useState([]);
  const [processes, setProcesses] = useState([]);
  const [requests, setRequests] = useState([]);

  useEffect(() => {
    socket.on('memoryData', (data) => {
      setMemoryData(data.pieChartData);
      setProcesses(data.processes);
      setRequests(data.requests);
    });

    return () => {
      socket.off('memoryData');
    };
  }, []);

  return (
    <div className="App">
      <h1>Dashboard</h1>
      <div className="charts">
        <MemoryPieChart data={memoryData} />
      </div>
      <div className="tables">
        <h2>Procesos</h2>
        <ProcessTable processes={processes} />
        <h2>Solicitudes</h2>
        <RequestTable requests={requests} />
      </div>
    </div>
  );
};

export default App;
