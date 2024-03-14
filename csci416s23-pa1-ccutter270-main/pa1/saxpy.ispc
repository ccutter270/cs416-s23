/**
 * @brief Compute SAXPY with IPSC without tasks
 *
 * @param n Array length
 * @param alpha "A" in SAXPY
 * @param x "X" operand array
 * @param y "Y" operand array
 */
export void SaxpyISPC(uniform int n, uniform float alpha, uniform float x[], uniform float y[]) {
  foreach (i = 0 ... n) {           
    y[i] = alpha * x[i] + y[i];
  }
}

task void SaxpyISPCTask(uniform int n, uniform float alpha, uniform float x[], uniform float y[], uniform int span) {
  // TODO: Implement the SAXPY task body
}

/**
 * @brief Compute SAXPY with IPSC with tasks
 *
 * @param n Array length
 * @param alpha "A" in SAXPY
 * @param x "X" operand array
 * @param y "Y" operand array
 * @param tasks Number of tasks for ISPC to execute
 */
export void SaxpyISPCTasks(uniform int n, uniform float alpha, uniform float x[], uniform float y[], uniform int tasks) {
  // TODO: Implement the SAXPY task launch for the specified number of tasks
}

