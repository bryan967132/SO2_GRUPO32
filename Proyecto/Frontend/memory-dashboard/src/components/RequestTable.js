import React from 'react';

const RequestTable = ({ requests }) => {
  return (
    <table>
      <thead>
        <tr>
          <th>ID</th>
          <th>Fecha</th>
          <th>Nombre</th>
          <th>PID</th>
          <th>Tamaño (MB)</th>
          <th>Tipo</th>
        </tr>
      </thead>
      <tbody>
        {requests.map((request, index) => (
          <tr key={index}>
            <td>{request.id}</td>
            <td>{new Date(request.fecha).toLocaleString()}</td>
            <td>{request.nombre}</td>
            <td>{request.pid}</td>
            <td>{parseFloat(request.tamano_mb).toFixed(2)}</td>
            <td>{request.tipo}</td>
          </tr>
        ))}
      </tbody>
    </table>
  );
};

export default RequestTable;
