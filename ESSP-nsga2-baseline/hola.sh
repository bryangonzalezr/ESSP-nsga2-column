#!/bin/bash

# Script para obtener puntos no dominados (Pareto-óptimos) de archivos apf_InstanceX.dat
# Los puntos están en formato: x y (separados por espacio)
# Un punto (x1,y1) domina a (x2,y2) si x1<=x2 Y y1<=y2 Y al menos una desigualdad es estricta

# Función para encontrar puntos no dominados
find_non_dominated() {
    local input_file="$1"
    local temp_result=$(mktemp)
    
    # Usar awk para implementar el algoritmo de Pareto más eficientemente
    awk '
    {
        x[NR] = int($1)
        y[NR] = int($2)
        n = NR
    }
    END {
        for (i = 1; i <= n; i++) {
            dominated = 0
            for (j = 1; j <= n; j++) {
                if (i != j) {
                    if ((x[j] <= x[i] && y[j] <= y[i]) && (x[j] < x[i] || y[j] < y[i])) {
                        dominated = 1
                        break
                    }
                }
            }
            if (!dominated) {
                printf("(%d, %d)\n", y[i], x[i])
            }
        }
    }' "$input_file" | sort -u > "$temp_result"

    
    # Mostrar resultado y limpiar archivo temporal
    cat "$temp_result"
    rm "$temp_result"
}

echo "Procesando archivos de instancias..."

# Procesar las 10 instancias
for i in {1..10}; do
    input_file="sols/Instance${i}.dat/apf_Instance${i}.dat"
    clean_file="sols/Instance${i}.dat/apf_Instance${i}_clean.dat"
    pareto_file="sols/Instance${i}.dat/apf_Instance${i}_pareto.dat"
    
    # Verificar si el archivo existe
    if [ ! -f "$input_file" ]; then
        echo "Advertencia: No se encontró el archivo $input_file"
        continue
    fi
    
    echo "Procesando $input_file..."
    
    # Paso 1: Eliminar duplicados
    sort "$input_file" | uniq > "$clean_file"
    
    # Paso 2: Encontrar puntos no dominados del archivo limpio
    find_non_dominated "$clean_file" > "$pareto_file"
    
    # Mostrar estadísticas
    original_count=$(wc -l < "$input_file")
    clean_count=$(wc -l < "$clean_file")
    pareto_count=$(wc -l < "$pareto_file")
    duplicates_removed=$((original_count - clean_count))
    dominated_removed=$((clean_count - pareto_count))
    
    echo "  - Puntos originales: $original_count"
    echo "  - Duplicados eliminados: $duplicates_removed"
    echo "  - Puntos únicos: $clean_count"
    echo "  - Puntos dominados eliminados: $dominated_removed"
    echo "  - Puntos no dominados (Pareto): $pareto_count"
    echo "  - Archivo limpio: $clean_file"
    echo "  - Archivo Pareto: $pareto_file"
    echo
done

echo "Proceso completado."
echo
echo "Archivos generados:"
echo "- *_clean.dat: Puntos únicos (sin duplicados)"
echo "- *_pareto.dat: Puntos no dominados (Pareto-óptimos)"