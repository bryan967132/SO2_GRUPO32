SELECT * FROM Memoria;

SELECT  pid,
        nombre,
        GREATEST(SUM(CASE WHEN tipo = 'mmap2' THEN tamano ELSE tamano * -1 END),0)/1024/1024 memoria_mb,
        ROUND(GREATEST(SUM(CASE WHEN tipo = 'mmap2' THEN tamano ELSE tamano * -1 END),0) /
            (SELECT SUM(porcentaje)
            FROM (SELECT GREATEST(SUM(CASE WHEN tipo = 'mmap2' THEN tamano ELSE tamano * -1 END), 0) porcentaje
                  FROM Memoria
                  GROUP BY pid, nombre) total
            ) * 100, 2) porcentaje
FROM Memoria
GROUP BY pid, nombre
ORDER BY pid