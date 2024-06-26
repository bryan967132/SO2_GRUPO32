import React from 'react';

const RequestTable = ({ requests }) => {
  return (
    <table>
      <thead>
        <tr>
          <th>PID</th>
          <th>Llamada</th>
          <th>Tamaño</th>
          <th>Fecha</th>
        </tr>
      </thead>
      <tbody>
        {requests.map((request, index) => (
          <tr key={index}>
            <td>{request.pid}</td>
            <td>{request.call}</td>
            <td>{request.size}</td>
            <td>{request.date}</td>
          </tr>
        ))}
      </tbody>
    </table>
  );
};

export default RequestTable;
