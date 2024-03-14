
static const float kTol = 0.00001f;  

/**
 * @brief Compute sqrt of vector floats using Newton's method using ISPC without tasks
 * 
 * @param n Length of input and output arrays
 * @param initial_guess Initial guess for iterative approximation
 * @param values Input array
 * @param result Output array
 */
export void SqrtISPC(uniform int n, uniform float initial_guess, uniform float values[], uniform float result[]) {
  foreach (i = 0 ... n) {

    float guess = initial_guess;
    float value = values[i];
  
    // Compute 1/sqrt(value) via Newton's method (to within a ratio of kTol)
    float ratio = (3.f - value * guess * guess) * 0.5;
    while (abs(ratio - 1.f) > kTol) {
      guess = guess * ratio;
      ratio = (3.f - value * guess * guess) * 0.5;
    }
    result[i] = value * guess;
  }
}

task void SqrtISPCTask(uniform int n, uniform float initial_guess, uniform float values[], uniform float result[], uniform int span) {
  uniform int start = taskIndex * span;
  uniform int end = min(n, start + span);


  foreach (i = start ... end) {

    float guess = initial_guess;
    float value = values[i];
  
    // Compute 1/sqrt(value) via Newton's method (to within a ratio of kTol)
    float ratio = (3.f - value * guess * guess) * 0.5;
    while (abs(ratio - 1.f) > kTol) {
      guess = guess * ratio;
      ratio = (3.f - value * guess * guess) * 0.5;
    }
    result[i] = value * guess;
  }
}

/**
 * @brief Compute sqrt of vector floats using Newton's method using ISPC with tasks
 * 
 * @param n Length of input and output arrays
 * @param initial_guess Initial guess for iterative approximation
 * @param values Input array
 * @param result Output array
 */
export void SqrtISPCTasks(uniform int n, uniform float initial_guess, uniform float values[], uniform float result[]) {
  // Use 288 tasks (36*8)
  uniform int span = (n+287) / 288;  
  launch[288] SqrtISPCTask(n, initial_guess, values, result, span);
}
