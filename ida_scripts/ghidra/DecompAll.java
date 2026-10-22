// Ghidra headless script: decompiles every function of the WL.EXE image (loaded at 1000:0000, DS = 1CE2h) into text.
// Arguments: <entries.txt> <output.txt>
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSpace;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Function;
import java.io.*;
import java.math.BigInteger;
import java.util.*;

public class DecompAll extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        List<Integer> entries = new ArrayList<>();
        try (BufferedReader r = new BufferedReader(new FileReader(args[0]))) {
            String line;
            while ((line = r.readLine()) != null) {
                line = line.split("#")[0].trim();
                if (!line.isEmpty()) entries.add(Integer.parseInt(line, 16));
            }
        }
        AddressSpace space = currentProgram.getAddressFactory().getDefaultAddressSpace();
        Register ds = currentProgram.getLanguage().getRegister("DS");
        Register es = currentProgram.getLanguage().getRegister("ES");
        Address lo = space.getAddress("1000:0000");
        Address hi = space.getAddress("1000:ffff");
        println("range " + lo + " " + hi + " ds=" + ds);
        currentProgram.getProgramContext().setValue(ds, lo, hi, BigInteger.valueOf(0x1CE2));
        for (int e : entries) {
            Address a = space.getAddress(String.format("1000:%04x", e));
            disassemble(a);
            if (getFunctionAt(a) == null) createFunction(a, String.format("fn_%04X", e));
        }
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        try (PrintWriter out = new PrintWriter(new FileWriter(args[1]))) {
            for (int e : entries) {
                Address a = space.getAddress(String.format("1000:%04x", e));
                Function f = getFunctionAt(a);
                if (f == null) { out.println("// no function at " + a); continue; }
                DecompileResults res = di.decompileFunction(f, 60, monitor);
                out.println("// ==== " + String.format("%04X", e) + " " + f.getName());
                if (res.decompileCompleted() && res.getDecompiledFunction() != null) out.println(res.getDecompiledFunction().getC());
                else out.println("// decompile failed: " + res.getErrorMessage());
            }
        }
    }
}
