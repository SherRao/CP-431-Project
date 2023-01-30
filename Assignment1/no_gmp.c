#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mpi.h"

#define MAX_PRIME 1000000000000
#define NUMBERS_BETWEEN_UPDATES 10000000
#define ulint unsigned long int
#define true 1
#define false 0
#define tag 1000

void mainProcess(int);
void childProcess(int, int);
void calculateLargestPrimeDiff(int, int, ulint *, ulint *, ulint *, ulint *, ulint *);
void collectResults(int);
ulint getNextPrime(ulint);
ulint isPrime(ulint);

/**
 *
 * Main entry function. Loads up MPI with the number of processes and the rank of the
 * current process, and calls functions accordingly.
 *
 */
int main(int argc, char **argv)
{
    int processes;
    int rank;
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS)
    {
        printf("MPI_Init failed!\n");
        exit(0);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
        mainProcess(processes);

    else
        childProcess(rank, processes);

    MPI_Finalize();
}

/**
 *
 * Function ran by the main process.
 *
 */
void mainProcess(int processes)
{
    collectResults(processes);
}

/**
 *
 * Function ran by child processes.
 *
 */
void childProcess(int rank, int processes)
{
    ulint smallestPrime;
    ulint largestPrime;
    ulint largestPrimeGapStart;
    ulint largestPrimeGapEnd;
    ulint largestPrimeGap;
    calculateLargestPrimeDiff(rank, processes, &smallestPrime, &largestPrime, &largestPrimeGapStart, &largestPrimeGapEnd, &largestPrimeGap);

    char message[1000];
    sprintf(message, "%lu,%lu,%lu,%lu,%lu", smallestPrime, largestPrime, largestPrimeGapStart, largestPrimeGapEnd, largestPrimeGap);
    MPI_Send(message, 100, MPI_CHAR, 0, tag, MPI_COMM_WORLD);
}

/**
 * Calculates the largest prime gap in the range of numbers by a specific process.
 *
 * @param rank The rank/ID of the process.
 * @param processes The number of total processes.
 * @param smallestPrime The smallest prime in the range.
 * @param largestPrime The largest prime in the range.
 * @param largestPrimeGapStart The start of the largest prime gap.
 * @param largestPrimeGapEnd The end of the largest prime gap.
 * @param largestPrimeGap The largest prime gap.
 *
 */
void calculateLargestPrimeDiff(int rank, int processes, ulint *smallestPrime, ulint *largestPrime,
                               ulint *largestPrimeGapStart, ulint *largestPrimeGapEnd, ulint *largestPrimeGap)
{

    ulint divisions = MAX_PRIME / processes;
    ulint rangeStart = (rank - 1) * divisions;
    ulint rangeEnd = rank * divisions;

    ulint primeGapStart = -1;
    ulint primeGapEnd = -1;
    ulint primeGap = -1;

    ulint a = getNextPrime(rangeStart);
    ulint b = getNextPrime(a);
    ulint lastPrimeUpdate = a;
    while (b < rangeEnd)
    {
        *largestPrime = b;
        int gap = b - a;

        if (gap > primeGap)
        {
            *largestPrimeGapStart = a;
            *largestPrimeGapEnd = b;
            *largestPrimeGap = gap;
        }

        a = b;
        b = getNextPrime(b);
        if (a - lastPrimeUpdate >= NUMBERS_BETWEEN_UPDATES)
        {
            lastPrimeUpdate = a;
            printf("Rank: %d || Current prime: %d, Largest prime gap: %d - %d || Gap: %d", rank, a, *largestPrimeGapStart, *largestPrimeGapEnd, *largestPrimeGap);
        }
    }
}

/**
 *
 * Collects the results from the child processes and prints the largest prime gap.
 *
 */
void collectResults(int processes)
{
    int resultStart;
    int resultEnd;
    int resultGap;

    int *edgePrimeStarts = malloc(sizeof(int) * processes);
    int *edgePrimeEnds = malloc(sizeof(int) * processes);
    int *primeGapStarts = malloc(sizeof(int) * processes);
    int *primeGapEnds = malloc(sizeof(int) * processes);
    int *primeGaps = malloc(sizeof(int) * processes);

    char message[1000];

    // Receive messages from child processes and store the data.
    for (int i = 1; i < processes; i++)
    {
        MPI_Recv(message, 100, MPI_CHAR, i, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        int rangeStart = atoi(strtok(message, ","));
        int rangeEnd = atoi(strtok(NULL, ","));
        int primeGapStart = atoi(strtok(NULL, ","));
        int primeGapEnd = atoi(strtok(NULL, ","));
        int primeGap = atoi(strtok(NULL, ","));
        printf("Rank: %d || Range: %d - %d || Prime gap: %d - %d || Gap: %d", i, rangeStart, rangeEnd, primeGapStart, primeGapEnd, primeGap);

        edgePrimeStarts[i] = rangeStart;
        edgePrimeEnds[i] = rangeEnd;
        primeGapStarts[i] = primeGapStart;
        primeGapEnds[i] = primeGapEnd;
        primeGaps[i] = primeGap;
    }

    // Find the largest prime gap.
    int largestPrimeGap = -1;
    int largestPrimeGapStart = -1;
    int largestPrimeGapEnd = -1;
    for (int i = 1; i < processes; i++)
    {
        if (primeGaps[i] > largestPrimeGap)
        {
            largestPrimeGap = primeGaps[i];
            largestPrimeGapStart = primeGapStarts[i];
            largestPrimeGapEnd = primeGapEnds[i];
        }
    }
    printf("Largest prime gap: %d - %d || Gap: %d", largestPrimeGapStart, largestPrimeGapEnd, largestPrimeGap);

    // Find the largest edge prime gap.
    // Ignore the first start edge prime gap.
    int largestEdgePrimeGap = -1;
    int largestEdgePrimeGapStart = -1;
    int largestEdgePrimeGapEnd = -1;
    for (int i = 0; i < processes - 1; i++)
    {
        int gap = edgePrimeEnds[i] - edgePrimeStarts[i + 1];
        if (gap > largestEdgePrimeGap)
        {
            largestEdgePrimeGap = gap;
            largestEdgePrimeGapStart = edgePrimeStarts[i];
            largestEdgePrimeGapEnd = edgePrimeEnds[i + 1];
        }
    }
    printf("Largest edge prime gap: %d - %d || Gap: %d", largestEdgePrimeGapStart, largestEdgePrimeGapEnd, largestEdgePrimeGap);

    // Compare the largest prime gap and the largest edge prime gap.
    if (largestPrimeGap > largestEdgePrimeGap)
    {
        resultGap = largestPrimeGap;
        resultStart = largestPrimeGapStart;
        resultEnd = largestPrimeGapEnd;
    }
    else
    {
        resultGap = largestEdgePrimeGap;
        resultStart = largestEdgePrimeGapStart;
        resultEnd = largestEdgePrimeGapEnd;
    }
    printf("Largest gap: %d - %d || Gap: %d", resultStart, resultEnd, resultGap);
}

/**
 *
 * Calculates the next prime number.
 *
 * @param x The number to start from.
 * @return The next prime number.
 *
 */
ulint getNextPrime(ulint x)
{
    ulint i = x + 1;
    while (!isPrime(i))
        i++;

    return i;
}

/**
 *
 * Checks if a number is prime.
 *
 * @param x The number to check.
 * @return True if the number is prime, false otherwise.
 *
 */
ulint isPrime(ulint x)
{
    if (x == 2)
        return true;

    if (x == 1 || x % 2 == 0)
        return false;

    ulint i = 2;
    while (i * i < x)
    {
        if (x % i == 0)
            return false;

        i++;
    }
    return true;
}