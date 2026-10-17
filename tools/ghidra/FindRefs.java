// Ghidra post-script: for each argument, list the functions that reference it.
// Arguments: hex addresses (e.g. 04000130), hex ranges (04000120-04000134), or str:<substring>.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

public class FindRefs extends GhidraScript {
    private void report(String label, Address target) {
        ReferenceIterator it = currentProgram.getReferenceManager().getReferencesTo(target);
        while (it.hasNext()) {
            Reference r = it.next();
            Function f = getFunctionContaining(r.getFromAddress());
            println(String.format("%s <- %s %s (%s)", label, r.getFromAddress(), r.getReferenceType(),
                f == null ? "no function" : f.getName() + "@" + f.getEntryPoint()));
        }
    }

    @Override
    public void run() throws Exception {
        for (String arg : getScriptArgs()) {
            if (arg.startsWith("str:")) {
                String needle = arg.substring(4);
                DataIterator di = currentProgram.getListing().getDefinedData(true);
                while (di.hasNext()) {
                    Data data = di.next();
                    if (!data.hasStringValue()) continue;
                    String s = data.getDefaultValueRepresentation();
                    if (s.contains(needle)) report("\"" + s + "\"@" + data.getAddress(), data.getAddress());
                }
            } else if (arg.contains("-")) {
                String[] p = arg.split("-");
                Address lo = toAddr(Long.parseLong(p[0], 16)), hi = toAddr(Long.parseLong(p[1], 16));
                for (Address a = lo; a.compareTo(hi) <= 0; a = a.add(1)) report(a.toString(), a);
            } else {
                Address a = toAddr(Long.parseLong(arg, 16));
                report(a.toString(), a);
            }
        }
    }
}
