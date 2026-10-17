// Ghidra pre-script: lay out the GBA memory map around a ROM loaded at 0x08000000,
// then mark the ROM entry point so auto-analysis has somewhere to start.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.SourceType;

public class GbaMemoryMap extends GhidraScript {

    private void block(String name, long start, long size, boolean write, boolean exec) throws Exception {
        Memory mem = currentProgram.getMemory();
        Address addr = toAddr(start);
        if (mem.getBlock(addr) != null) {
            return;
        }
        MemoryBlock b = mem.createUninitializedBlock(name, addr, size, false);
        b.setRead(true);
        b.setWrite(write);
        b.setExecute(exec);
        b.setVolatile(name.equals("IO"));
    }

    @Override
    public void run() throws Exception {
        block("BIOS",  0x00000000L, 0x4000,  false, true);
        block("EWRAM", 0x02000000L, 0x40000, true,  true);
        block("IWRAM", 0x03000000L, 0x8000,  true,  true);
        block("IO",    0x04000000L, 0x400,   true,  false);
        block("PAL",   0x05000000L, 0x400,   true,  false);
        block("VRAM",  0x06000000L, 0x18000, true,  false);
        block("OAM",   0x07000000L, 0x400,   true,  false);
        block("SRAM",  0x0E000000L, 0x10000, true,  false);

        MemoryBlock rom = currentProgram.getMemory().getBlock(toAddr(0x08000000L));
        if (rom != null) {
            rom.setName("ROM");
            rom.setWrite(false);
            rom.setExecute(true);
        }

        Address entry = toAddr(0x08000000L);
        createLabel(entry, "_rom_entry", true, SourceType.USER_DEFINED);
        addEntryPoint(entry);
        disassemble(entry);

        println("GBA memory map applied");
    }
}
