DROP DATABASE IF EXISTS Proyecto1;
CREATE DATABASE Proyecto1;
USE Proyecto1;

CREATE TABLE Memoria(
    id INT PRIMARY KEY AUTO_INCREMENT,
    pid INT,
    nombre VARCHAR(100),
    tipo VARCHAR(15),
    tamano BIGINT,
    fecha TIMESTAMP
);