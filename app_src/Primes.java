
public class Primes { 
    public static void main(String[] args) {
    	int N = Integer.parseInt(args[0]);
        boolean[] isPrime = new boolean[N];
        for (int i = 2; i < N; i++) isPrime[i] = true;
        
        for (int i = 2; i < N; i++) {        	
        	// Remove numbers where i is its factor
        	for (int j = i; j*i < N; j++) 
                isPrime[i*j] = false;
        }

        int outputCnt = 0;
        final int number_each_line = 10;
        for (int i = 2; i < N; i++) {
        	if (isPrime[i]) {
        		System.out.print(' ');
        		System.out.print(i);
   		
        		++outputCnt;
        		if (outputCnt >= number_each_line) {
        			System.out.println();
        			outputCnt = 0;
        		}
        	}
        }
        System.out.println();
    }
}