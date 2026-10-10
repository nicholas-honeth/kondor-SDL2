// Apply Borland TDInfo-derived types, names, signatures and locals to KONDOR.EXE, fix
// mis-resolved near calls, then decompile every game function into per-source-file drafts.
// Args: spec.json outdir
//@category Kondor
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.data.DataUtilities.ClearDataMode;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import com.google.gson.*;
import java.io.*;
import java.util.*;

public class KondorApply extends GhidraScript {
    static final int CODESEG = 0x1488, DATASEG = 0x25b2;
    static final long CODE_LO = 0x14880, CODE_HI = 0x22dd0;
    DataTypeManager dtm;
    CategoryPath cat = new CategoryPath("/kondor");
    Map<String, DataType> named = new HashMap<>();
    SegmentedAddressSpace space;

    Address seg(int s, long off) { return space.getAddress(s, (int) off); }
    Address lin(long a) {
        if (a >= 0x25550) return seg(0x2555, a - 0x25550);
        if (a >= CODE_HI) return seg(0x22dd, a - CODE_HI);
        return a >= CODE_LO ? seg(CODESEG, a - CODE_LO) : seg(0x1000, a - 0x10000);
    }

    DataType resolve(String t) {
        if (t.startsWith("ptr:")) return new PointerDataType(resolve(t.substring(4)), 4, dtm);
        if (t.startsWith("arr:")) {
            String[] p = t.split(":", 3);
            DataType e = resolve(p[2]);
            int n = Math.max(1, Integer.parseInt(p[1]));
            return new ArrayDataType(e, n, e.getLength(), dtm);
        }
        if (t.startsWith("struct:") || t.startsWith("enum:")) {
            DataType d = named.get(t.substring(t.indexOf(':') + 1));
            return d != null ? d : Undefined1DataType.dataType;
        }
        switch (t) {
            case "char": return CharDataType.dataType;
            case "uchar": return UnsignedCharDataType.dataType;
            case "int": return ShortDataType.dataType;          // Borland int = 16 bit
            case "uint": return UnsignedShortDataType.dataType;
            case "long": return IntegerDataType.dataType;        // 32 bit on x86-16 data org? force below
            case "ulong": return UnsignedIntegerDataType.dataType;
            case "float": return FloatDataType.dataType;
            case "double": return DoubleDataType.dataType;
            case "ldouble": return new Float10DataType(dtm);
            default: return VoidDataType.dataType;
        }
    }

    // Make sure "long" really is 4 bytes regardless of the program's data organization.
    DataType fixLen(DataType d, String t) {
        if ((t.equals("long") || t.equals("ulong")) && d.getLength() != 4)
            return t.equals("long") ? SignedDWordDataType.dataType : DWordDataType.dataType;
        if ((t.equals("int") || t.equals("uint")) && d.getLength() != 2)
            return t.equals("int") ? SignedWordDataType.dataType : WordDataType.dataType;
        return d;
    }
    DataType R(String t) { return fixLen(resolve(t), t); }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        JsonObject spec = JsonParser.parseReader(new FileReader(args[0])).getAsJsonObject();
        File outdir = new File(args[1]);
        outdir.mkdirs();
        dtm = currentProgram.getDataTypeManager();
        space = (SegmentedAddressSpace) currentProgram.getAddressFactory().getDefaultAddressSpace();
        SymbolTable st = currentProgram.getSymbolTable();
        Listing listing = currentProgram.getListing();

        // 1. types: create all, then fill (so forward references resolve)
        JsonArray types = spec.getAsJsonArray("types");
        for (JsonElement je : types) {
            JsonObject t = je.getAsJsonObject();
            String name = t.get("name").getAsString();
            if (name.equals("FILE")) name = "FILE_bc";
            DataType d;
            if (t.get("kind").getAsString().equals("enum")) {
                EnumDataType en = new EnumDataType(cat, name, 2, dtm);
                for (Map.Entry<String, JsonElement> v : t.getAsJsonObject("values").entrySet())
                    en.add(v.getKey(), v.getValue().getAsLong());
                d = en;
            } else {
                d = new StructureDataType(cat, name, t.get("size").getAsInt(), dtm);
            }
            named.put(t.get("name").getAsString(), dtm.addDataType(d, DataTypeConflictHandler.REPLACE_HANDLER));
        }
        for (JsonElement je : types) {
            JsonObject t = je.getAsJsonObject();
            if (!t.get("kind").getAsString().equals("struct")) continue;
            Structure s = (Structure) named.get(t.get("name").getAsString());
            for (JsonElement fe : t.getAsJsonArray("fields")) {
                JsonObject f = fe.getAsJsonObject();
                DataType d = R(f.get("type").getAsString());
                try {
                    s.replaceAtOffset(f.get("off").getAsInt(), d, d.getLength(), f.get("name").getAsString(), null);
                } catch (Exception ex) {
                    println("field " + s.getName() + "." + f.get("name").getAsString() + ": " + ex.getMessage());
                }
            }
        }
        println("types applied: " + named.size());

        // 2. globals (DGROUP was partly disassembled as code by auto-analysis: clear it first)
        clearListing(seg(DATASEG, 0), seg(DATASEG, 0xf46f));
        int gok = 0;
        for (JsonElement je : spec.getAsJsonArray("globals")) {
            JsonObject g = je.getAsJsonObject();
            int off = g.get("off").getAsInt();
            if (off < 0) continue;
            Address a = seg(DATASEG, off);
            String nm = g.get("name").getAsString();
            try {
                st.createLabel(a, nm, SourceType.USER_DEFINED).setPrimary();
                DataType d = R(g.get("type").getAsString());
                if (d.getLength() > 0)
                    DataUtilities.createData(currentProgram, a, d, -1, ClearDataMode.CLEAR_ALL_CONFLICT_DATA);
                gok++;
            } catch (Exception ex) {
                println("global " + nm + ": " + ex.getMessage());
            }
        }
        println("globals applied: " + gok);

        // 3. fix near calls in game code: Ghidra mis-wraps rel16 targets across the 64K offset boundary
        int fixed = 0;
        InstructionIterator it = listing.getInstructions(new AddressSet(lin(CODE_LO), lin(CODE_HI - 1)), true);
        ReferenceManager rm = currentProgram.getReferenceManager();
        List<Address> callTargets = new ArrayList<>();
        while (it.hasNext()) {
            Instruction in = it.next();
            byte[] b = in.getBytes();
            if ((b[0] & 0xff) != 0xE8 || b.length != 3) continue;
            long ip = in.getAddress().getOffset() - CODE_LO;
            int rel = (short) ((b[1] & 0xff) | (b[2] & 0xff) << 8);
            long tgt = CODE_LO + ((ip + 3 + rel) & 0xffff);
            Address[] flows = in.getFlows();
            if (flows.length == 1 && flows[0].getOffset() == tgt) continue;
            Address ta = lin(tgt);
            for (Reference r : in.getReferencesFrom()) rm.delete(r);
            Reference r = rm.addMemoryReference(in.getAddress(), ta, RefType.CALL_OVERRIDE_UNCONDITIONAL,
                    SourceType.USER_DEFINED, 0);
            rm.setPrimary(r, true);
            callTargets.add(ta);
            fixed++;
        }
        println("near calls overridden: " + fixed);

        // 4. functions: create, name (with class namespace), signature, locals
        Map<String, Namespace> classes = new HashMap<>();
        List<Function> ours = new ArrayList<>();
        Map<Function, JsonObject> meta = new LinkedHashMap<>();
        for (JsonElement je : spec.getAsJsonArray("functions")) {
            JsonObject f = je.getAsJsonObject();
            long addr = f.get("addr").getAsLong();
            Address a = lin(addr);
            Function fn = getFunctionAt(a);
            if (fn == null) {
                disassemble(a);
                fn = createFunction(a, null);
            }
            if (fn == null) { println("no function at " + a + " " + f.get("full").getAsString()); continue; }
            String full = f.get("full").getAsString();
            String bare = full.indexOf('(') < 0 ? full : full.substring(0, full.indexOf('('));
            String cls = null, nm = bare;
            if (bare.contains("::")) { cls = bare.substring(0, bare.indexOf("::")); nm = bare.substring(bare.indexOf("::") + 2); }
            if (nm.isEmpty() || nm.startsWith("$")) nm = f.get("name").getAsString();
            nm = nm.replace("~", "dtor_");
            try {
                if (cls != null) {
                    Namespace ns = classes.get(cls);
                    if (ns == null) {
                        ns = st.getNamespace(cls, currentProgram.getGlobalNamespace());
                        if (ns == null) ns = st.createClass(currentProgram.getGlobalNamespace(), cls, SourceType.USER_DEFINED);
                        classes.put(cls, ns);
                    }
                    fn.setParentNamespace(ns);
                }
                fn.setName(nm, SourceType.USER_DEFINED);
            } catch (Exception ex) { println("name " + full + ": " + ex.getMessage()); }
            meta.put(fn, f);
            if (addr >= CODE_LO) ours.add(fn);
        }
        for (Map.Entry<Function, JsonObject> en : meta.entrySet()) {
            Function fn = en.getKey(); JsonObject f = en.getValue();
            if (f.get("mangled").getAsString().endsWith("$qve")) continue;   // interrupt handlers
            // top-level scope vars only (scope == function entry), dedup by name
            TreeMap<Integer, JsonObject> params = new TreeMap<>();
            Map<String, JsonObject> locals = new LinkedHashMap<>();
            StringBuilder regs = new StringBuilder();
            for (JsonElement ve : f.getAsJsonArray("vars")) {
                JsonObject v = ve.getAsJsonObject();
                String c = v.get("class").getAsString();
                int bp = v.get("bp").getAsInt();
                if (c.equals("AUTO") && bp > 0) params.putIfAbsent(bp, v);
                else if (c.equals("AUTO")) locals.putIfAbsent(v.get("name").getAsString() + "@" + bp, v);
                else if (c.equals("REGISTER"))
                    regs.append(" ").append(v.get("type").getAsString()).append(" ").append(v.get("name").getAsString())
                        .append(bp == 6 ? "=SI" : bp == 7 ? "=DI" : "=r" + bp);
            }
            try {
                List<ParameterImpl> ps = new ArrayList<>();
                for (JsonObject v : params.values())
                    ps.add(new ParameterImpl(v.get("name").getAsString(), R(v.get("type").getAsString()), currentProgram));
                fn.updateFunction("__cdecl16far", new ReturnParameterImpl(R(f.get("ret").getAsString()), currentProgram),
                        ps, FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.USER_DEFINED);
            } catch (Exception ex) { println("sig " + f.get("full").getAsString() + ": " + ex.getMessage()); }
            StackFrame fr = fn.getStackFrame();
            for (JsonObject v : locals.values()) {
                int off = -2 + v.get("bp").getAsInt();   // bp = entry SP - 2 for far procs
                try {
                    DataType d = R(v.get("type").getAsString());
                    Variable old = fr.getVariableContaining(off);
                    if (old != null && old.getStackOffset() == off) fr.clearVariable(off);
                    fr.createVariable(v.get("name").getAsString(), off, d, SourceType.USER_DEFINED);
                } catch (Exception ex) { /* overlapping/odd locals: leave to decompiler */ }
            }
            String plate = f.get("full").getAsString() + "\n" + f.get("src").getAsString() + " lines "
                    + f.getAsJsonArray("lines").toString() + (regs.length() > 0 ? "\nregister vars:" + regs : "");
            fn.setComment(plate);
        }
        println("functions: " + meta.size());

        // 5. decompile everything, grouped by original source file
        DecompInterface ifc = new DecompInterface();
        DecompileOptions opt = new DecompileOptions();
        ifc.setOptions(opt);
        ifc.toggleCCode(true);
        ifc.openProgram(currentProgram);
        Map<String, StringBuilder> bySrc = new TreeMap<>();
        int ok = 0, bad = 0;
        for (Function fn : ours) {
            JsonObject f = meta.get(fn);
            DecompileResults res = ifc.decompileFunction(fn, 180, monitor);
            String src = f.get("src").getAsString().replace("F:\\TC\\INCLUDE\\", "");
            StringBuilder sb = bySrc.computeIfAbsent(src, k -> new StringBuilder());
            sb.append("\n/* ==== ").append(f.get("full").getAsString()).append("  @").append(fn.getEntryPoint())
              .append("  lines ").append(f.getAsJsonArray("lines")).append(" ==== */\n");
            if (res != null && res.decompileCompleted()) {
                sb.append(res.getDecompiledFunction().getC()); ok++;
            } else {
                sb.append("/* decompile failed: ").append(res == null ? "null" : res.getErrorMessage()).append(" */\n"); bad++;
            }
        }
        for (Map.Entry<String, StringBuilder> e : bySrc.entrySet())
            try (Writer w = new FileWriter(new File(outdir, e.getKey() + ".c"))) { w.write(e.getValue().toString()); }
        println("decompiled ok=" + ok + " failed=" + bad);
    }
}
