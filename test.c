/* Find the biggest prime factor of a number */

int main() {

	int i;
	int max;
	int n = 329475;

	/* test 2 separately */
	if(n % 2 == 0) {
		max = 2;
		n = n / 2;
		while(n % 2 == 0) {
			n = n / 2;
		}
	}

	/* test 3 separately */
	if(n % 3 == 0) {
		max = 3;
		n = n / 3;
		while(n % 3 == 0) {
			n = n / 3;
		}
	}

	i = 5;
	/* go up to root with +6 step */
	while(i * i <= n) {

		/* Check once for i and once for i+2 */
		/* to cover all numbers in the form 6*k +/- 1. */
		/* Note that you could easily compress that in a loop. */

		/* i is prime divisor */
		if(n % i == 0) {
			/* i is the new max as it always increases */
			max = i;
			
			/* i might divide n multiple times */
			n = n / i;
			while(n % i == 0) {
				n = n / i;
			}
		}

		/* i+2 is prime divisor */
		if(n % (i+2) == 0) {
			/* i+2 is the new max as it always increases */
			max = i+2;
			
			n = n / (i+2);
			/* i+2 might divide n multiple times */
			while(n % (i+2) == 0) {
				n = n / (i+2);
			}
		}

		i = i + 6;
	}
	
	if(n > 1) {
		/* biggest prime factor was bigger than */
		/* the last computed root. */
		max = n;
	}

	print max;

	return 0;
}
