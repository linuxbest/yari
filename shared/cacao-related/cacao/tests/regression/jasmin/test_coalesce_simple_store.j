.class public test_coalesce_simple_store
.super java/lang/Object

; ======================================================================

.method public <init>()V
   aload_0
   invokenonvirtual java/lang/Object/<init>()V
   return
.end method

; ======================================================================

.method public static checkI(I)V
	.limit locals 1
	.limit stack 10
	getstatic java/lang/System/out Ljava/io/PrintStream;
	iload_0
	invokevirtual java/io/PrintStream/println(I)V
	return
.end method

; ======================================================================

.method public static main([Ljava/lang/String;)V
	.limit stack 2
	.limit locals 2

	ldc 35
	istore 1

	aload 0
	ifnull force_basic_block_boundary

	; --------------------------------------------------

	ldc 42
	istore 1

	; --------------------------------------------------

force_basic_block_boundary:

	iload 1
	invokestatic test_coalesce_simple_store/checkI(I)V
	; OUTPUT: 42

	return
.end method
