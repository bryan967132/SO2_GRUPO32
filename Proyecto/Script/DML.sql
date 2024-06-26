SELECT * FROM Memoria;

SELECT pid, nombre, GREATEST(SUM(CASE WHEN tipo = 'mmap2' THEN tamano ELSE tamano*-1 END),0) memoria
FROM Memoria
GROUP BY pid, nombre