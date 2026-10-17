// Ghidra post-script: decompile the functions at the given hex addresses (or all functions if the
// first argument is "all") into <outdir>/<address>_<name>.c
import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.address.Address;
import ghidra.program.model.lang.Register;
import ghidra.program.model.lang.RegisterValue;
import java.math.BigInteger;

public class DecompileFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) throw new IllegalArgumentException("usage: DecompileFunctions.java <outdir> <all|addr...>");
        File out = new File(args[0]);
        out.mkdirs();

        List<Function> fns = new ArrayList<>();
        if (args[1].equals("all")) {
            FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
            while (it.hasNext()) fns.add(it.next());
        } else {
            for (int i = 1; i < args.length; i++) {
                Address a = toAddr(Long.parseLong(args[i], 16) & ~1L);
                Function f = getFunctionAt(a);
                if (f == null) {
                    // not seeded: create a Thumb function there (vtable targets are only reachable through RAM)
                    if (getInstructionAt(a) == null) {
                        Register tmode = currentProgram.getRegister("TMode");
                        currentProgram.getProgramContext().setRegisterValue(a, a, new RegisterValue(tmode, BigInteger.ONE));
                        disassemble(a);
                    }
                    f = createFunction(a, null);
                    println((f == null ? "could not create function at " : "created function at ") + args[i]);
                }
                if (f != null) fns.add(f);
            }
        }

        DecompInterface ifc = new DecompInterface();
        ifc.openProgram(currentProgram);
        int ok = 0;
        for (Function f : fns) {
            DecompileResults res = ifc.decompileFunction(f, 60, monitor);
            String name = String.format("%08x_%s.c", f.getEntryPoint().getOffset(), f.getName());
            try (PrintWriter w = new PrintWriter(new FileWriter(new File(out, name)))) {
                if (res.decompileCompleted()) {
                    w.print(res.getDecompiledFunction().getC());
                    ok++;
                } else {
                    w.println("// decompile failed: " + res.getErrorMessage());
                }
            }
        }
        ifc.dispose();
        println("decompiled " + ok + "/" + fns.size() + " functions into " + out);
    }
}
