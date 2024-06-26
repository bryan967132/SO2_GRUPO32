import React from 'react';
import { Pie } from 'react-chartjs-2';

const MemoryPieChart = ({ data }) => {
  const pieData = {
    labels: data.map(d => d.name),
    datasets: [{
      data: data.map(d => d.memory),
      backgroundColor: ['#FF6384', '#36A2EB', '#FFCE56', '#FF5733', '#C70039', '#900C3F', '#581845', '#FFC300', '#DAF7A6', '#FFC0CB'],
    }],
  };

  return <Pie data={pieData} />;
};

export default MemoryPieChart;
