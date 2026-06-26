// greatest common divisor
public class GCD {
	static int gcd(int M, int N) {
	    if (N == 0) return M;
	    return gcd(N, M % N);
	}
	
	public static void main(String[] args) {
		int N = Integer.parseInt(args[0]);
		int M = Integer.parseInt(args[1]);
		System.out.println("Greatest Common Divisor (GCD) is:");
		System.out.println(gcd(N, M));
	}
}
