/**
 * @brief Compute sqrt of vector floats using Newton's method
 * 
 * @param n Length of input and output arrays
 * @param initial_guess Initial guess for iterative approximation
 * @param values Input array
 * @param result Output array
 */
void SqrtSerial(int n, float initial_guess, float values[], float result[]);

/**
 * @brief Compute sqrt of vector floats using Newton's method using SIMD intrinsics
 * 
 * @param n Length of input and output arrays
 * @param initial_guess Initial guess for iterative approximation
 * @param values Input array
 * @param result Output array
 */
void SqrtIntrinsics(int n, float initial_guess, float values[], float result[]);

