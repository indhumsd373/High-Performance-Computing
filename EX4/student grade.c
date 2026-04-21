#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME 50
#define NUM_STUDENTS 4 // Total students to process

// Define the Student structure
typedef struct {
    char name[MAX_NAME];
    int roll_no;
    float total_marks;
    char grade;
} Student;

// Function to determine grade based on marks
char calculate_grade(float marks) {
    if (marks >= 90) return 'A';
    else if (marks >= 80) return 'B';
    else if (marks >= 70) return 'C';
    else if (marks >= 60) return 'D';
    else return 'F';
}

int main(int argc, char** argv) {
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Ensure the number of students is divisible by the number of processes
    if (NUM_STUDENTS % size != 0) {
        if (rank == 0) printf("Error: Number of students (%d) must be divisible by number of processes (%d).\n", NUM_STUDENTS, size);
        MPI_Finalize();
        return 0;
    }

    int students_per_proc = NUM_STUDENTS / size;

    // --- STEP 1: Create the MPI Derived Datatype for 'Student' ---
    MPI_Datatype mpi_student_type;
    int blocklengths[4] = {MAX_NAME, 1, 1, 1};
    MPI_Aint displacements[4];
    MPI_Datatype types[4] = {MPI_CHAR, MPI_INT, MPI_FLOAT, MPI_CHAR};

    // Calculate displacements of members within the struct
    displacements[0] = offsetof(Student, name);
    displacements[1] = offsetof(Student, roll_no);
    displacements[2] = offsetof(Student, total_marks);
    displacements[3] = offsetof(Student, grade);

    MPI_Type_create_struct(4, blocklengths, displacements, types, &mpi_student_type);
    MPI_Type_commit(&mpi_student_type);

    Student *all_students = NULL;
    Student *local_students = (Student*)malloc(students_per_proc * sizeof(Student));

    // --- STEP 2: Root process reads from input file ---
    if (rank == 0) {
        all_students = (Student*)malloc(NUM_STUDENTS * sizeof(Student));

        // Reading from "input_students.txt"
        // Format: Name RollNo Marks
        FILE *fp = fopen("input_students.txt", "r");
        if (fp == NULL) {
            printf("Error: Could not open input_students.txt. Creating dummy data.\n");
            for(int i = 0; i < NUM_STUDENTS; i++) {
                sprintf(all_students[i].name, "Student_%d", i+1);
                all_students[i].roll_no = 100 + i;
                all_students[i].total_marks = 65.0 + (i * 10.0);
                all_students[i].grade = '?';
            }
        } else {
            for (int i = 0; i < NUM_STUDENTS; i++) {
                fscanf(fp, "%s %d %f", all_students[i].name, &all_students[i].roll_no, &all_students[i].total_marks);
                all_students[i].grade = '?';
            }
            fclose(fp);
        }
    }

    // --- STEP 3: Distribute data among processes ---
    MPI_Scatter(all_students, students_per_proc, mpi_student_type,
                local_students, students_per_proc, mpi_student_type,
                0, MPI_COMM_WORLD);

    // --- STEP 4: Each process calculates grades ---
    for (int i = 0; i < students_per_proc; i++) {
        local_students[i].grade = calculate_grade(local_students[i].total_marks);
        printf("Process %d calculated grade '%c' for %s\n", rank, local_students[i].grade, local_students[i].name);
    }

    // --- STEP 5: Gather results back to root ---
    MPI_Gather(local_students, students_per_proc, mpi_student_type,
               all_students, students_per_proc, mpi_student_type,
               0, MPI_COMM_WORLD);

    // --- STEP 6: Root process writes to output file ---
    if (rank == 0) {
        FILE *out_fp = fopen("grades.txt", "w");
        fprintf(out_fp, "Name\tRollNo\tMarks\tGrade\n");
        fprintf(out_fp, "-------------------------------------\n");
        for (int i = 0; i < NUM_STUDENTS; i++) {
            fprintf(out_fp, "%s\t%d\t%.2f\t%c\n",
                    all_students[i].name, all_students[i].roll_no,
                    all_students[i].total_marks, all_students[i].grade);
        }
        fclose(out_fp);
        printf("\nSuccess! Results written to 'grades.txt'.\n");
        free(all_students);
    }

    free(local_students);
    MPI_Type_free(&mpi_student_type);
    MPI_Finalize();
    return 0;
}
