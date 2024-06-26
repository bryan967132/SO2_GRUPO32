import React from 'react';

const ProcessTable = ({ processes }) => {
  return (
    <table>
      <thead>
        <tr>
          <th>PID</th>
          <th>Nombre</th>
          <th>Memoria (MB)</th>
          <th>Porcentaje</th>
        </tr>
      </thead>
      <tbody>
        {processes.map((process, index) => (
          <tr key={index}>
            <td>{process.pid}</td>
            <td>{process.nombre}</td>
            <td>{parseFloat(process.memoria_mb).toFixed(2)}</td>
            <td>{process.porcentaje}%</td>
          </tr>
        ))}
      </tbody>
    </table>
  );
};

export default ProcessTable;
