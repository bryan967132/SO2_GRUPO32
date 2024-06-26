import React, { useEffect, useState } from 'react';
import io from 'socket.io-client';
import MemoryPieChart from './components/MemoryPieChart';
import ProcessTable from './components/ProcessTable';
import RequestTable from './components/RequestTable';
import './App.css';

const socket = io('http://localhost:3001');

const App = () => {
  const [memoryData, setMemoryData] = useState([]);
  const [processes, setProcesses] = useState([]);
  const [requests, setRequests] = useState([]);
  const [error, setError] = useState(null);

  useEffect(() => {
    // Fetch data from endpoint on initial load
    const fetchData = async () => {
      try {
        const response = await fetch('http://localhost:3001/api/memory-data');
        const data = await response.json();
        setMemoryData(data.pieChartData);
        setProcesses(data.processes);
        setRequests(data.requests);
      } catch (error) {
        setError('No se pudo obtener los datos del servidor');
      }
    };

    fetchData();

    // Set up socket connection for real-time updates
    socket.on('connect_error', () => {
      setError('No se pudo conectar con el servidor');
    });

    socket.on('memoryData', (data) => {
      setError(null); // Clear error if data is received successfully
      setMemoryData(data.pieChartData);
      setProcesses(data.processes);
      setRequests(data.requests);
    });

    socket.on('disconnect', () => {
      setError('Desconectado del servidor');
    });

    return () => {
      socket.off('memoryData');
      socket.off('connect_error');
      socket.off('disconnect');
    };
  }, []);

  return (
    <div className="App">
      <h1>Dashboard</h1>
      {error && <p className="error">{error}</p>}
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
