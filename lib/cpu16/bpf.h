// Basic processor functions

U0 GC_REBOOT(GC* gc) {
  gc->r.PC = 0x0000; // Reset the program counter
  gc->r.PS = 0b01000000; // Reset flags (but enable interrupts)

  return 0;
}
