/* Code from Spring 2012 CSCI2100B+S 
   By Matthew 
   =D
*/

#include <stdio.h>

static inline void swap(int *a, int *b){
	int tmp = *a;
	*a = *b;
	*b = tmp;
}

static inline int median3(int *a, int left, int right){
	int center = (left + right) / 2;

	if (a[left] > a[center])
		swap(&a[left], &a[center]);
	if (a[left] > a[right])
		swap(&a[left], &a[right]);
	if (a[center] > a[right])
		swap(&a[center], &a[right]);
	// we are sure that: 
	// a[left] <= a[center] <= a[right]
	swap(&a[center], &a[right]); // hide pivot
	return a[right]; // return pivot
}

static void qsort_r(int *a, int left, int right){
	int i, j, p;
	// handle simple case first
	if (left >= right) return;
	if (left + 1 == right){
		if (a[left] > a[right])
			swap(&a[left], &a[right]);
		return;
	}
	p = median3(a, left, right);
	i = left; j = right;

	while (1){
		while (a[++i] < p);
		while (a[--j] > p);
		if (i < j)
			swap(&a[i], &a[j]);
		else
			break;
	} // restore pivot
	swap(&a[i], &a[right]); 
	
	qsort_r(a, left, i - 1);
	qsort_r(a, i + 1, right);
}

void sort(int n, int *a){
	qsort_r(a, 0, n - 1);
}

