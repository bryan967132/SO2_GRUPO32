import React from 'react';

const ProcessTable = ({ processes }) => {
  return (
    <table>
      <thead>
        <tr>
          <th>PID</th>
          <th>Nombre</th>
          <th>Memoria</th>
          <th>Porcentaje</th>
        </tr>
      </thead>
      <tbody>
        {processes.map((process, index) => (
          <tr key={index}>
            <td>{process.pid}</td>
            <td>{process.name}</td>
            <td>{process.memory}</td>
            <td>{process.percentage}%</td>
          </tr>
        ))}
      </tbody>
    </table>
  );
};

export default ProcessTable;
