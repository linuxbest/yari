public class fintest2 {
	public int num;
	
	public fintest2 (int n) { num = n; }

	public void finalize () {
		System.out.print ("finalized ");
		System.out.print (num);
		System.out.println (". object");
		}

	public static void main (String[] s) {
		int i;
		fintest2 f=null;
		
		Runtime.getRuntime().runFinalizersOnExit(true);

		for (i=0; i<100; i++) {
			System.out.print (i);
			System.out.println (". Objekt wird angelegt");
			f = new fintest2(i);
			}

		Runtime.getRuntime().exit(0);
	
		}
		
	}

	
