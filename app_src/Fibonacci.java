
public class Fibonacci {
	static int F(int i) { 
	    if (i < 1) return 0;
	    if (i == 1) return 1;
	    return F(i-1) + F(i-2);
	}
	
	public static void main(String[] args) {
		int N = Integer.parseInt(args[0]);
		System.out.println(F(N));
	}
}
