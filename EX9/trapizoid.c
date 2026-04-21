#include <stdio.h>
#include <omp.h>

double f(double x) {
    return x * x;
}

double Local_trap(double a, double b, int n, double h) {
    double integral = (f(a) + f(b)) / 2.0;
    for (int i = 1; i <= n - 1; i++) {
        integral += f(a + i * h);
    }
    return integral * h;
}

int main() {
    double a, b;
    int n;
    int thread_count = 4;

    // Get input from user
    printf("Enter a (lower limit): ");
    scanf("%lf", &a);
    printf("Enter b (upper limit): ");
    scanf("%lf", &b);
    printf("Enter n (number of trapezoids): ");
    scanf("%d", &n);

    double h = (b - a) / n;

    // --- 1. Reduction Method ---
    double global_result = 0.0;
    #pragma omp parallel for num_threads(thread_count) reduction(+: global_result)
    for (int i = 1; i < n; i++) {
        global_result += f(a + i * h);
    }
    global_result = h * (global_result + (f(a) + f(b)) / 2.0);
    printf("\n--- Method 1: Reduction ---\n");
    printf("Global Result: %f\n\n", global_result);

    // --- 2. Critical Method with Thread Logging ---
    printf("--- Method 2: Critical Section (Local Results) ---\n");
    double global_result_critical = 0.0;

    #pragma omp parallel num_threads(thread_count)
    {
        int my_rank = omp_get_thread_num();
        int local_n = n / thread_count;

        // Calculate this thread's local range
        double local_a = a + my_rank * local_n * h;
        double local_b = local_a + local_n * h;

        double my_result = Local_trap(local_a, local_b, local_n, h);

        printf("Thread %d | Range: [%.2f, %.2f] | Local Result: %f\n",
                my_rank, local_a, local_b, my_result);

        #pragma omp critical
        global_result_critical += my_result;
    }

    printf("Final Global Result (Critical): %f\n", global_result_critical);

    return 0;
}
