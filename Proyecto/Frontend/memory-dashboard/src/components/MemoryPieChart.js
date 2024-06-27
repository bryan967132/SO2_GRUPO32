import React from 'react';
import { Pie } from 'react-chartjs-2';
import { Chart as ChartJS, ArcElement, Tooltip, Legend } from 'chart.js';

// Registrar los componentes
ChartJS.register(ArcElement, Tooltip, Legend);

const MemoryPieChart = ({ data }) => {
  const pieData = {
    labels: data.map(d => `${d.name} (${d.percentage}%)`),
    datasets: [{
      data: data.map(d => d.memory),
      backgroundColor: [
        '#FF6384', '#36A2EB', '#FFCE56', '#FF5733', '#C70039', 
        '#900C3F', '#581845', '#FFC300', '#DAF7A6', '#FFC0CB',
        '#888888' // Color para "Otros"
      ],
    }],
  };

  return <Pie data={pieData} />;
};

export default MemoryPieChart;
