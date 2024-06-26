from flask import Flask, jsonify
from flask_cors import CORS
from mysql import connector
import dotenv
import os


app = Flask(__name__)
CORS(app)

dotenv.load_dotenv(os.path.join(os.path.dirname(__file__), '..', '.env'))

try:
    db = connector.connect(
        host=os.getenv('MYSQL_HOST'),
        user=os.getenv('MYSQL_USER'),
        password=os.getenv('MYSQL_PASS'),
        database=os.getenv('MYSQL_DB')
    )
except connector.Error as e:
    print(e)

cursor = db.cursor()

@app.route('/api/procesos', methods=['GET'])
def get_procesos():
    if db is None:
        return jsonify({'error': 'No se pudo conectar a la base de datos'})
    
    cursor.execute('''
            SELECT  pid,
                    nombre,
                    GREATEST(SUM(CASE WHEN tipo = 'mmap2' THEN tamano ELSE tamano * -1 END),0)/1024/1024 memoria_mb,
                    ROUND(GREATEST(SUM(CASE WHEN tipo = 'mmap2' THEN tamano ELSE tamano * -1 END),0) /
                        (SELECT SUM(porcentaje)
                        FROM (SELECT GREATEST(SUM(CASE WHEN tipo = 'mmap2' THEN tamano ELSE tamano * -1 END),0) porcentaje
                            FROM Memoria
                            GROUP BY pid, nombre) total
                        ) * 100, 2) porcentaje
            FROM Memoria
            GROUP BY pid, nombre
            ORDER BY pid
        ''')
    
    colums = [desc[0] for desc in cursor.description]
    result = [dict(zip(colums, row)) for row in cursor.fetchall()]
    return jsonify(result)

@app.route('/api/solicitudes', methods=['GET'])
def get_solicitudes():
    if db is None:
        return jsonify({'error': 'No se pudo conectar a la base de datos'})
    
    cursor.execute('''
            SELECT  id,
                    pid,
                    nombre,
                    tipo,
                    tamano tamano_bytes,
                    ROUND(GREATEST(tamano/1024/1024,0), 2) tamano_mb,
                    fecha
            FROM Memoria
            ORDER BY id
        ''')
    
    colums = [desc[0] for desc in cursor.description]
    result = [dict(zip(colums, row)) for row in cursor.fetchall()]
    return jsonify(result)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)