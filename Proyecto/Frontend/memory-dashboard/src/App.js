import React, { useEffect, useState } from 'react';
import MemoryPieChart from './components/MemoryPieChart';
import ProcessTable from './components/ProcessTable';
import RequestTable from './components/RequestTable';
import './App.css';

const App = () => {
  const [memoryData, setMemoryData] = useState([]);
  const [processes, setProcesses] = useState([]);
  const [requests, setRequests] = useState([]);
  const [error, setError] = useState(null);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const responseProcesos = await fetch('http://18.191.237.75:5000/api/procesos');
        const dataProcesos = await responseProcesos.json();
        setProcesses(dataProcesos);

        const transformedPieChartData = dataProcesos.map(process => ({
          name: process.nombre,
          memory: parseFloat(process.memoria_mb),
          percentage: process.porcentaje
        }));
        setMemoryData(transformedPieChartData);

        const responseSolicitudes = await fetch('http://18.191.237.75:5000/api/solicitudes');
        const dataSolicitudes = await responseSolicitudes.json();
        setRequests(dataSolicitudes);
      } catch (error) {
        setError('No se pudo obtener los datos del servidor');
      }
    };

    fetchData();

    const interval = setInterval(fetchData, 2000); // Actualiza cada 2 segundos

    return () => clearInterval(interval); // Limpia el intervalo al desmontar el componente
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
