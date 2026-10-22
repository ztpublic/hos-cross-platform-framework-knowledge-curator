"""Read saved evidence and write recommendation coverage; never run source apps."""

import csv
import hashlib
import json
from collections import Counter
from pathlib import Path

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[2]
PRIOR = ROOT / "maintenance/investigations/2026-10-21-feature-loss"

OPPORTUNITIES = {
    "M1": "Feature audit and capability investigation",
    "M2": "Complete-project compatibility solver",
    "M3": "Native dependency porting foundry",
    "M4": "Native adapter factory and capability transplantation",
    "M5": "Build, package and deployment doctor",
    "M6": "Cross-language causal debugger and lifetime investigator",
    "M7": "Reference-app behavior twin and feature-parity oracle",
    "M8": "Lifecycle and system-interaction chaos lab",
    "M9": "Desktop experience, rendering and performance adaptation lab",
    "M10": "Data continuity and interoperability auditor",
    "M11": "Continuous upstream synchronization agent",
    "M12": "Executable migration knowledge and regression benchmark",
}

# Editorial routing by mechanism area, refined for specific catalog entries.
# It is not a finding of independent incidents, effort or measured priority.
AREA_ROUTES = {
    "integration": ["M4", "M6"], "storage": ["M10"],
    "window": ["M9", "M8"], "web": ["M4", "M6", "M8"],
    "lifecycle": ["M8", "M6"], "media": ["M4", "M9", "M8"],
    "networking": ["M10", "M8"], "network": ["M10", "M8"],
    "build": ["M2", "M3", "M5"], "architecture": ["M1", "M4"],
    "host": ["M4", "M5"], "native": ["M3", "M6"],
    "compatibility": ["M1", "M2"], "security": ["M1", "M10"],
    "permissions": ["M1", "M4"], "ui": ["M9"], "ffi": ["M6"],
    "plugins": ["M2", "M4", "M5"], "channels": ["M4", "M6"],
    "accessibility": ["M9", "M7"], "input": ["M9"],
    "rendering": ["M9"], "tooling": ["M5", "M6"],
}
OVERRIDES = {
    "embedding-patches-and-dependency-order": ["M11", "M2", "M5"],
    "flutter-ohos-target-platform-vendoring": ["M11", "M2", "M9"],
    "shared-source-platform-detection": ["M11", "M2"],
    "plugin-graph-source-and-api": ["M2", "M4", "M11"],
    "flutter-engine-cache-coherence": ["M2", "M5"],
    "qt-qpa-runtime-template-coherence": ["M2", "M5", "M6"],
    "qt-ohosextras-version-boundary": ["M2", "M4"],
    "kmp-ohos-compose-target": ["M2", "M4", "M11"],
    "kmp-ohos-dependency-and-renderer": ["M2", "M9"],
    "electron-ohos-native-addon-abi": ["M2", "M3", "M5"],
    "electron-ohos-older-runtime-shims": ["M2", "M4", "M11"],
    "capability-gated-backend-initialization": ["M1", "M4", "M8"],
    "privileged-capability-gating": ["M1", "M5"],
    "jit-executable-memory-authorization": ["M1", "M5"],
    "hap-executable-declaration": ["M1", "M5", "M3"],
    "hap-signing-identity": ["M5", "M10"],
    "hap-minimum-api-compatibility": ["M1", "M2", "M5"],
    "system-metric-unavailability": ["M1", "M4"],
    "electron-ohos-non-pty-exec": ["M1", "M4", "M8"],
    "native-code-load-location": ["M3", "M5", "M1"],
    "native-dependency-closure": ["M3", "M5"],
    "native-dependency-completeness": ["M3", "M5"],
    "native-linker-toolchain-workarounds": ["M3", "M5"],
    "ffi-library-and-symbol-availability": ["M3", "M5", "M7"],
    "native-event-loop-handle-teardown": ["M6", "M8"],
    "napi-thread-and-handle-lifetime": ["M6", "M8"],
    "native-ffi-shared-buffer-ownership": ["M6", "M8", "M9"],
    "native-registration-and-handler-coverage": ["M4", "M6", "M5"],
    "database-backend-and-data-contract": ["M10", "M2", "M11"],
    "secure-storage-backend-selection": ["M10", "M4"],
    "qt-ohos-external-clipboard-paste": ["M4", "M7", "M6"],
    "file-export-contract": ["M10", "M7", "M4"],
    "document-picker-uri-access": ["M10", "M4", "M8"],
    "system-reminder-identity-and-lifetime": ["M4", "M8", "M7"],
    "continuous-background-task-contract": ["M4", "M8"],
    "qt-ohos-kde-shims": ["M1", "M3", "M4", "M7"],
    "ocr-feature-evolution": ["M1", "M4", "M11"],
}
FINDING_ROUTES = {
    "M1": ["F01", "F04", "F07", "F09", "F10", "F13", "F22", "F27", "F30", "F32"],
    "M2": ["F02", "F15", "F25"],
    "M3": ["F02", "F04", "F08", "F17", "F19"],
    "M4": ["F03", "F06", "F08", "F11", "F12", "F15", "F16", "F18", "F20", "F21", "F23", "F24", "F26", "F28", "F29", "F30", "F33"],
    "M7": [f"F{i:02d}" for i in range(1, 34)],
    "M8": ["F07", "F12", "F15", "F18", "F20", "F23", "F28", "F29", "F31"],
    "M9": ["F04", "F05", "F06", "F10", "F14", "F19", "F20", "F22", "F32", "F33"],
    "M10": ["F03", "F06", "F08", "F13", "F18", "F26", "F31"],
    "M11": ["F05", "F06", "F11", "F24", "F25"],
}


def read_json(path):
    return json.loads(path.read_text())


def ordered(ids):
    return sorted(set(ids), key=lambda item: int(item[1:]))


def main():
    with (ROOT / "index.csv").open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    row_ids = {row["id"] for row in rows}
    assert set(OVERRIDES) <= row_ids
    entries = []
    for row in rows:
        body = (ROOT / row["path"]).read_text()
        checks = body.split("## Check it works", 1)[1].strip()
        entries.append({
            "id": row["id"], "title": row["title"], "area": row["area"],
            "frameworks": row["frameworks"].split("|") if row["frameworks"] else [],
            "problem": row["when_to_use"], "mechanism": row["summary"],
            "acceptance_plan_from_entry": checks,
            "path": row["path"], "provenance": row["provenance"],
            "evidence_tier": row["evidence_tier"], "pc_validation": row["pc_validation"],
            "opportunities": OVERRIDES.get(row["id"], AREA_ROUTES[row["area"]]),
        })
    by_id = {entry["id"]: entry for entry in entries}
    direct = [s for s in read_json(ROOT / "maintenance/sources.json")["sources"] if "framework" in s]
    flutter = read_json(ROOT / "provenance/snapshots/flutter/projects.json")["projects"]
    with (ROOT / "maintenance/import-map.csv").open(newline="") as stream:
        imports = {r["source_path"]: r["entry_ids"].split("|") for r in csv.DictReader(stream) if r["collection"] == "flutter"}
    provenance = {p.stem: read_json(p) for p in (ROOT / "provenance").glob("*.json")}
    previous_coverage = {p["id"]: p for p in read_json(PRIOR / "coverage.json")["projects"]}
    findings = read_json(PRIOR / "findings.json")["findings"]
    finding_ids = {f["id"] for f in findings}
    assert all(set(ids) <= finding_ids for ids in FINDING_ROUTES.values())
    projects = []
    for source_type, records in [("direct-source-registration", direct), ("imported-flutter-project-record", flutter)]:
        for source in records:
            project_id = source["id"]
            if source_type == "direct-source-registration":
                linked = {entry_id for entry_id, p in provenance.items() if any(s.get("collection") == project_id for s in p.get("sources", []))}
                revision = source["analyzed_commit"]
                original = []
            else:
                original = source.get("knowledge_ids", [])
                linked = {entry for old_id in original for entry in imports.get(f"knowledge/{old_id}.md", [old_id]) if entry in row_ids}
                revision = source.get("analyzed_head")
            assert linked <= row_ids
            associated = [f for f in findings if f["project"] == project_id]
            opportunities = [item for entry_id in linked for item in by_id[entry_id]["opportunities"]]
            opportunities += [opp for opp, ids in FINDING_ROUTES.items() if any(f["id"] in ids for f in associated)]
            projects.append({
                "id": project_id, "registration_type": source_type,
                "framework": source.get("framework", "flutter"),
                "repository": source.get("url", source.get("repository")),
                "recorded_revision": revision,
                "knowledge_entries": sorted(linked),
                "original_imported_knowledge_ids": original,
                "selected_feature_findings": [f["id"] for f in associated],
                "opportunities": ordered(opportunities),
                "prior_feature_review_disposition": previous_coverage[project_id]["coverage"],
                "prior_screen_limitation": previous_coverage[project_id].get("screen_limitation"),
                "source_review_this_proposal": "Saved registrations, catalog associations and selected existing evidence; no fresh full repository audit.",
            })
    assert len(entries) == 187 and len(projects) == 80
    assert len({p["id"] for p in projects}) == len(projects)
    assert all(p["knowledge_entries"] and p["opportunities"] for p in projects)
    assert set(previous_coverage) == {p["id"] for p in projects}
    paths = [ROOT / "index.csv", ROOT / "maintenance/sources.json", ROOT / "maintenance/import-map.csv", ROOT / "provenance/snapshots/flutter/projects.json"]
    paths += [PRIOR / name for name in ["REPORT.md", "coverage.json", "findings.json", "evidence.json", "official-sources.json"]]
    paths += [ROOT / entry[key] for entry in entries for key in ["path", "provenance"]]
    payload = {
        "schema_version": 1, "date": "2026-10-21", "timezone": "Asia/Shanghai",
        "scope": "Product recommendations from saved catalog and project evidence, not source updates or runtime validation.",
        "routing_method": "Editorial area-based routing with explicit mechanism-specific overrides; M7 applies conceptually to all selected feature-loss cases. Counts do not measure independent incidents or priority.",
        "counts": {"knowledge_entries": len(entries), "direct_projects": len(direct), "imported_flutter_projects": len(flutter), "registered_project_identities": len(projects), "selected_feature_cases": len(findings)},
        "evidence_tiers": dict(Counter(e["evidence_tier"] for e in entries)),
        "pc_validation": dict(Counter(e["pc_validation"] for e in entries)),
        "opportunities": OPPORTUNITIES, "knowledge_entries": entries, "projects": projects,
        "feature_findings": [{**f, "opportunities": ordered([opp for opp, ids in FINDING_ROUTES.items() if f["id"] in ids]), "evidence_origin": "Reused saved investigation; not re-reproduced here."} for f in findings],
        "input_sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
        "limits": ["No source-project setup/build/sign/install was performed.", "No new physical-device behavior was reproduced.", "No source checkpoints were advanced.", "Imported collection project histories have partial coverage; prior dispositions are retained.", "Prior investigation had 59 completed Flutter keyword screens and one partial screen; screening does not establish parity.", "The author's existing accessibility QA tool was not audited.", "Common mechanism applicability does not establish runtime verification in all frameworks."],
    }
    (OUT / "coverage.json").write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps(payload["counts"], indent=2))


if __name__ == "__main__":
    main()
