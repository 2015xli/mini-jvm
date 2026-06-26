/* 
 * http://en.wikipedia.org/wiki/Josephus_problem
 */
class Josephus {
	private int val;
	private Josephus next;
	
	Josephus(int v) { val = v; }
    
    public static void main(String[] args) {
    	int N = Integer.parseInt(args[0]);
        int M = Integer.parseInt(args[1]);
        
        // Initialize circle
        Josephus t = new Josephus(1);
        Josephus x = t;
        for (int i = 2; i <= N; i++)
          x = (x.next = new Josephus(i));
        x.next = t;
        
        
        // Execute Josephus permutation, now x is last participant at circle
        while (x != x.next) {
        	// Search person who is prior to participant at interval position
        	for (int i = 1; i < M; i++) x = x.next;
        	
        	// Del
            x.next = x.next.next;
        }
        
        System.out.print("The last survivor is ");
        System.out.println(x.val);
      }
  }