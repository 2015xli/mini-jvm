
public class Factorial {
	static int factorial(int N) {
		if (N == 0) return 1;
	    return N*factorial(N-1);
	}
	
	public static void main(String[] args) {
		int N = Integer.parseInt(args[0]);
		System.out.println(factorial(N));
	}
}
