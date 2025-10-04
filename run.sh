#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "Usage: $0 <runs> [max_parallel_jobs]"
  echo "  runs: Number of runs per instance"
  echo "  max_parallel_jobs: Maximum number of instances to run in parallel (default: number of CPU cores)"
  exit 1
fi

RUN=$1
MAX_PARALLEL=${2:-$(nproc)}  # Default to number of CPU cores
RUNMODE=0

echo "Running with maximum $MAX_PARALLEL instances in parallel"

# Base folders
NSGA2_FOLDER="ESSP-nsga2-baseline"
HV_FOLDER="hv-1.3-src"
INSTANCES_FOLDER="instances"
OUTPUT_BASE=${NSGA2_FOLDER}/"sols"
PARAMS_FILE="parametros_instancias.txt"

# Fixed parameters (define these as needed)
FIXED_POP_SIZE=52        # You can adjust this
FIXED_EVALUACIONES=100000   # Fixed number of evaluations

# ========================================
# DEFAULT PARAMETERS CONFIGURATION
# ========================================
# Define all default parameters in one place for easy management
declare -A DEFAULT_PARAMS=(
    ["INITIAL_GENERATION_TYPE"]="0"
    ["ILS_RESET"]="10"
    ["LS_ITERATIONS"]="1000"
    ["MIBE_BLOCK_SIZE"]="3"
    ["MIBS_BLOCK_SIZE"]="3"
    ["MUTAMOUNT"]="0.1"
    ["POP_SIZE"]="52"
    ["CROSSOVER_PROB"]="0.1"
    ["CROSS1"]="0.7"
    ["CROSS2"]="0.7"
    ["MUTATION_PROB"]="0.4"
    ["MUTATION_1"]="0.6"
    ["MUTATION_2"]="0"
    ["MUTATION_3"]="0.2"
    ["MUTATION_4"]="0"
    ["MUTATION_5"]="0.8"
    ["PMO"]="0.8"
)

# Function to get parameter value with default fallback
get_param_value() {
    local param_name="$1"
    local param_value="$2"
    local default_value="${DEFAULT_PARAMS[$param_name]}"
    
    # If parameter is empty or not provided, use default
    if [[ -z "$param_value" || "$param_value" == "null" || "$param_value" == "NULL" ]]; then
        echo "$default_value"
    else
        # Convert decimal separators and return the value
        echo "$param_value" | sed 's/,/./g'
    fi
}

# Function to set all parameters to defaults
set_default_params() {
    INITIAL_GENERATION_TYPE="${DEFAULT_PARAMS[INITIAL_GENERATION_TYPE]}"
    ILS_RESET="${DEFAULT_PARAMS[ILS_RESET]}"
    LS_ITERATIONS="${DEFAULT_PARAMS[LS_ITERATIONS]}"
    MIBE_BLOCK_SIZE="${DEFAULT_PARAMS[MIBE_BLOCK_SIZE]}"
    MIBS_BLOCK_SIZE="${DEFAULT_PARAMS[MIBS_BLOCK_SIZE]}"
    MUTAMOUNT="${DEFAULT_PARAMS[MUTAMOUNT]}"
    POP_SIZE="${DEFAULT_PARAMS[POP_SIZE]}"
    CROSSOVER_PROB="${DEFAULT_PARAMS[CROSSOVER_PROB]}"
    CROSS1="${DEFAULT_PARAMS[CROSS1]}"
    CROSS2="${DEFAULT_PARAMS[CROSS2]}"
    MUTATION_PROB="${DEFAULT_PARAMS[MUTATION_PROB]}"
    MUTATION_1="${DEFAULT_PARAMS[MUTATION_1]}"
    MUTATION_2="${DEFAULT_PARAMS[MUTATION_2]}"
    MUTATION_3="${DEFAULT_PARAMS[MUTATION_3]}"
    MUTATION_4="${DEFAULT_PARAMS[MUTATION_4]}"
    MUTATION_5="${DEFAULT_PARAMS[MUTATION_5]}"
    PMO="${DEFAULT_PARAMS[PMO]}"
}

# Function to display current parameter configuration
display_params() {
    local instance_name="$1"
    echo "Parameters for $instance_name:"
    echo "  INITIAL_GENERATION_TYPE: $INITIAL_GENERATION_TYPE (default: ${DEFAULT_PARAMS[INITIAL_GENERATION_TYPE]})"
    echo "  Population Size: $POP_SIZE (default: ${DEFAULT_PARAMS[POP_SIZE]})"
    echo "  Generations: $GENERATIONS"
    echo "  Fixed Evaluations: $FIXED_EVALUACIONES"
    echo "  Actual Evaluations: $ACTUAL_EVALUATIONS"
    echo "  ILS Reset: $ILS_RESET (default: ${DEFAULT_PARAMS[ILS_RESET]})"
    echo "  LS Iterations: $LS_ITERATIONS (default: ${DEFAULT_PARAMS[LS_ITERATIONS]})"
    echo "  MIBE Block Size: $MIBE_BLOCK_SIZE (default: ${DEFAULT_PARAMS[MIBE_BLOCK_SIZE]})"
    echo "  MIBS Block Size: $MIBS_BLOCK_SIZE (default: ${DEFAULT_PARAMS[MIBS_BLOCK_SIZE]})"
    echo "  Crossover Prob: $CROSSOVER_PROB (default: ${DEFAULT_PARAMS[CROSSOVER_PROB]})"
    echo "  Mutation Prob: $MUTATION_PROB (default: ${DEFAULT_PARAMS[MUTATION_PROB]})"
    echo "  Mutation Amount: $MUTAMOUNT (default: ${DEFAULT_PARAMS[MUTAMOUNT]})"
    echo "  PMO: $PMO (default: ${DEFAULT_PARAMS[PMO]})"
}

# Ensure output base folder exists
mkdir -p "$OUTPUT_BASE"

# Check if parameters file exists
if [ ! -f "$PARAMS_FILE" ]; then
  echo "Error: Parameters file '$PARAMS_FILE' not found!"
  exit 1
fi

# Read reference points from optimos.txt
declare -A REF_POINTS
while IFS= read -r line || [ -n "$line" ]; do
  INSTANCE_NAME=$(echo "$line" | awk '{print $1}')
  P1=$(echo "$line" | awk '{print $2}')
  P2=$(echo "$line" | awk '{print $3}')
  REF_POINTS["$INSTANCE_NAME"]="$P1 $P2"
done < "optimos.txt"

# Read parameters from the parameters file with the correct format
# CSV format: Instancia,mutamount,p,pc,pm,pm1,pm2,pm3,pm4,pm5
declare -A INSTANCE_PARAMS
while IFS=',' read -r instance mutamount p pc pm pm1 pm2 pm3 pm4 pm5 || [ -n "$instance" ]; do
  # Skip empty lines and header
  if [[ -z "$instance" || "$instance" == "Instancia" || "$instance" == "Instancias" ]]; then
    continue
  fi
  
  # Store parameters for this instance in the order they appear in CSV
  # Only storing the parameters that are actually in your CSV file
  INSTANCE_PARAMS["$instance"]="$mutamount,$p,$pc,$pm,$pm1,$pm2,$pm3,$pm4,$pm5"
done < "$PARAMS_FILE"

# Function to calculate generations based on population size and fixed evaluations
calculate_generations() {
  local population_size="$1"
  local evaluaciones="$2"
  
  # Calculate generations: evaluaciones / population_size
  # Use bc for floating point division and round down
  local generations=$(echo "scale=0; $evaluaciones / $population_size" | bc)
  echo "$generations"
}

# Enhanced function to parse parameters for an instance with robust defaults
parse_params() {
  local instance="$1"
  local params="${INSTANCE_PARAMS[$instance]}"
  
  if [ -z "$params" ]; then
    echo "Warning: No parameters found for instance $instance, using all defaults"
    set_default_params
  else
    # Parse the CSV line - only the parameters that exist in your CSV
    IFS=',' read -r raw_mutamount raw_p raw_pc raw_pm raw_pm1 raw_pm2 raw_pm3 raw_pm4 raw_pm5 <<< "$params"
    
    # Apply CSV parameters (these are the ones from your file)
    MUTAMOUNT=$(get_param_value "MUTAMOUNT" "$raw_mutamount")
    POP_SIZE=$(get_param_value "POP_SIZE" "$raw_p")
    CROSSOVER_PROB=$(get_param_value "CROSSOVER_PROB" "$raw_pc")
    MUTATION_PROB=$(get_param_value "MUTATION_PROB" "$raw_pm")
    MUTATION_1=$(get_param_value "MUTATION_1" "$raw_pm1")
    MUTATION_2=$(get_param_value "MUTATION_2" "$raw_pm2")
    MUTATION_3=$(get_param_value "MUTATION_3" "$raw_pm3")
    MUTATION_4=$(get_param_value "MUTATION_4" "$raw_pm4")
    MUTATION_5=$(get_param_value "MUTATION_5" "$raw_pm5")
    
    # Set parameters NOT in CSV to their defaults
    INITIAL_GENERATION_TYPE="${DEFAULT_PARAMS[INITIAL_GENERATION_TYPE]}"
    ILS_RESET="${DEFAULT_PARAMS[ILS_RESET]}"
    LS_ITERATIONS="${DEFAULT_PARAMS[LS_ITERATIONS]}"
    MIBE_BLOCK_SIZE="${DEFAULT_PARAMS[MIBE_BLOCK_SIZE]}"
    MIBS_BLOCK_SIZE="${DEFAULT_PARAMS[MIBS_BLOCK_SIZE]}"
    CROSS1="${DEFAULT_PARAMS[CROSS1]}"
    CROSS2="${DEFAULT_PARAMS[CROSS2]}"
    PMO="${DEFAULT_PARAMS[PMO]}"
  fi
  
  # Calculate generations based on population size and fixed evaluations
  GENERATIONS=$(calculate_generations "$POP_SIZE" "$FIXED_EVALUACIONES")
}

# Function to process a single instance
process_instance() {
  local INSTANCE_NAME="$1"
  local INSTANCE_FILE="$INSTANCES_FOLDER/$INSTANCE_NAME"
  
  # Check if instance file exists
  if [ ! -f "$NSGA2_FOLDER/$INSTANCE_FILE" ]; then
    echo "Warning: Instance file $INSTANCE_FILE not found, skipping..."
    return 1
  fi
  
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] Starting instance: $INSTANCE_NAME (PID: $$)"
  OUTPUT_FOLDER="$OUTPUT_BASE/${INSTANCE_NAME}"

  # Create output folder for the instance
  mkdir -p "$OUTPUT_FOLDER"
  mkdir -p "$OUTPUT_FOLDER/allout"

  # Parse parameters for this instance
  parse_params "$INSTANCE_NAME"
  
  # Calculate actual evaluations (for verification)
  ACTUAL_EVALUATIONS=$(echo "$POP_SIZE * $GENERATIONS" | bc)
  
  # Display parameters with defaults information
  display_params "$INSTANCE_NAME"

  # Initialize summary CSV file for the current instance
  SUMMARY_CSV="$OUTPUT_FOLDER/summary.csv"
  echo "Instance,Run,Hypervolume,ExecutionTime,InitializationTime,PopulationSize,Generations,ActualEvaluations" > "$SUMMARY_CSV"

  # Change to NSGA2 folder for this instance
  cd "$NSGA2_FOLDER"

  # Run the nsga2r program for the specified number of runs
  RUN_NUMBER=1
  echo "Running $RUN runs for instance $INSTANCE_NAME"
  while (( $(echo "$RUN_NUMBER <= $RUN" | bc -l) )); do
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Run $RUN_NUMBER for instance $INSTANCE_NAME"
    CURRENT_SEED=$(echo "0.1 + 0.01*$RUN_NUMBER" | bc)

    # Capture start time for nsga2r execution
    START_TIME=$(date +%s)

    # Build and display the command
    CMD="./nsga2r $CURRENT_SEED instances/$INSTANCE_NAME $POP_SIZE $GENERATIONS $OBJECTIVES"
    CMD="$CMD $INITIAL_GENERATION_TYPE $LS_ITERATIONS $ILS_RESET $MIBE_BLOCK_SIZE $MIBS_BLOCK_SIZE"
    CMD="$CMD $CROSSOVER_PROB $MUTATION_PROB $MUTATION_1 $MUTATION_2 $MUTATION_3 $MUTATION_4"
    CMD="$CMD $MUTATION_5 $PMO $MUTAMOUNT $CROSS1 $CROSS2 $RUN_NUMBER $RUNMODE"
    
    echo "Executing: $CMD"
    
    # Run nsga2r program
    ./nsga2r "$CURRENT_SEED" "instances/$INSTANCE_NAME" "$POP_SIZE" "$GENERATIONS" \
         "$OBJECTIVES" "$INITIAL_GENERATION_TYPE" "$LS_ITERATIONS" \
         "$ILS_RESET" "$MIBE_BLOCK_SIZE" "$MIBS_BLOCK_SIZE" \
         "$CROSSOVER_PROB" "$MUTATION_PROB" "$MUTATION_1" \
         "$MUTATION_2" "$MUTATION_3" "$MUTATION_4" "$MUTATION_5" "$PMO" \
         "$MUTAMOUNT" "$CROSS1" "$CROSS2" "$RUN_NUMBER" "$RUNMODE" \
         > "../$OUTPUT_FOLDER/allout/nsga2r_output_run_$RUN_NUMBER.txt"

    # Capture end time for nsga2r execution
    END_TIME=$(date +%s)
    EXECUTION_TIME=$((END_TIME - START_TIME))

    # Extract initialization time
    INIT_TIME=$(grep "Time taken for initialization = " "../$OUTPUT_FOLDER/allout/nsga2r_output_run_$RUN_NUMBER.txt" | awk '{print $6}')

    # Calculate hypervolume
    OF_FILE="../$OUTPUT_FOLDER/allout/of_${RUN_NUMBER}.out"
    REF_POINT="${REF_POINTS[$INSTANCE_NAME]}"
    echo "Calculating hypervolume for $OF_FILE with reference point $REF_POINT"
    if [ ! -s "$OF_FILE" ]; then
      HYPERVOLUME=0
    else
      HYPERVOLUME=$(../$HV_FOLDER/hv -r "$REF_POINT" "$OF_FILE" | grep "Hypervolume" | awk '{print $2}')
    fi

    # Append to summary CSV for the current instance (including new columns)
    echo "$INSTANCE_NAME,$RUN_NUMBER,$HYPERVOLUME,$EXECUTION_TIME,$INIT_TIME,$POP_SIZE,$GENERATIONS,$ACTUAL_EVALUATIONS" >> "../$SUMMARY_CSV"

    # Increment run number
    RUN_NUMBER=$((RUN_NUMBER + 1))
  done

  # Return to original directory
  cd ..

  # Aggregate `of_{run}.out` files into `apf_{instance_name}`
  echo "Aggregating data for $INSTANCE_NAME..."
  APF_FILE="$OUTPUT_FOLDER/apf_${INSTANCE_NAME}"
  > "$APF_FILE"  # Create or clear the apf file
  for OF_FILE in "$OUTPUT_FOLDER/allout"/of_*.out; do
    if [ -f "$OF_FILE" ]; then
      echo "Processing file: $OF_FILE"
      cat "$OF_FILE" >> "$APF_FILE"
    fi
  done

  echo "[$(date '+%Y-%m-%d %H:%M:%S')] Completed instance $INSTANCE_NAME. Output saved to $APF_FILE."
}

# Fixed parameters
SEED=0.1
OBJECTIVES=2

# Export functions and variables needed by background processes
export -f process_instance
export -f parse_params
export -f calculate_generations
export -f get_param_value
export -f set_default_params
export -f display_params
export RUN RUNMODE NSGA2_FOLDER HV_FOLDER INSTANCES_FOLDER OUTPUT_BASE
export FIXED_POP_SIZE FIXED_EVALUACIONES SEED OBJECTIVES
export -A INSTANCE_PARAMS REF_POINTS DEFAULT_PARAMS

# Create array of instances to process
INSTANCES=()
for INSTANCE_NAME in "${!INSTANCE_PARAMS[@]}"; do
  INSTANCES+=("$INSTANCE_NAME")
done

echo "Found ${#INSTANCES[@]} instances to process"
echo "Instances: ${INSTANCES[*]}"
echo "Fixed evaluations per instance: $FIXED_EVALUACIONES"

# Display default parameters configuration
echo ""
echo "========================================="
echo "DEFAULT PARAMETERS CONFIGURATION"
echo "========================================="
for param in "${!DEFAULT_PARAMS[@]}"; do
    echo "$param = ${DEFAULT_PARAMS[$param]}"
done
echo "========================================="
echo ""

# Process instances in parallel with job control
ACTIVE_JOBS=0
INSTANCE_INDEX=0
TOTAL_INSTANCES=${#INSTANCES[@]}

# Function to wait for a job to complete
wait_for_job() {
  wait -n  # Wait for any background job to complete
  ACTIVE_JOBS=$((ACTIVE_JOBS - 1))
}

echo "[$(date '+%Y-%m-%d %H:%M:%S')] Starting parallel processing..."

# Launch initial batch of jobs
while [ $INSTANCE_INDEX -lt $TOTAL_INSTANCES ] && [ $ACTIVE_JOBS -lt $MAX_PARALLEL ]; do
  INSTANCE_NAME="${INSTANCES[$INSTANCE_INDEX]}"
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] Launching instance $((INSTANCE_INDEX + 1))/$TOTAL_INSTANCES: $INSTANCE_NAME"
  
  process_instance "$INSTANCE_NAME" &
  
  ACTIVE_JOBS=$((ACTIVE_JOBS + 1))
  INSTANCE_INDEX=$((INSTANCE_INDEX + 1))
done

# Continue launching jobs as others complete
while [ $INSTANCE_INDEX -lt $TOTAL_INSTANCES ]; do
  wait_for_job
  
  INSTANCE_NAME="${INSTANCES[$INSTANCE_INDEX]}"
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] Launching instance $((INSTANCE_INDEX + 1))/$TOTAL_INSTANCES: $INSTANCE_NAME"
  
  process_instance "$INSTANCE_NAME" &
  
  ACTIVE_JOBS=$((ACTIVE_JOBS + 1))
  INSTANCE_INDEX=$((INSTANCE_INDEX + 1))
done

# Wait for remaining jobs to complete
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Waiting for remaining $ACTIVE_JOBS jobs to complete..."
while [ $ACTIVE_JOBS -gt 0 ]; do
  wait_for_job
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $ACTIVE_JOBS jobs remaining..."
done

echo "[$(date '+%Y-%m-%d %H:%M:%S')] All instances completed!"
echo "Processing completed for all instances with custom parameters."
echo "Instance-specific summary CSV files are saved in their respective folders."