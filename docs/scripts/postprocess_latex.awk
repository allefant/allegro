# Post-process LaTeX documents generated by Pandoc so cross-references to
# API entries work properly.  We need to do two things:
#
# - insert labels for each API entry section
# - replace dummy references by real references
#
# See also the make_dummy_refs script.
#

# Strip CR terminators in case files have DOS terminators.
{ sub(/\r$/, "") }

# Comment out unneeded packages (tetex 3.0 doesn't include them).
/^\\usepackage.*{(amsmath|ucs|inputenc)}/ {
    print "%" $0
    next
}

# Insert \label{id} for all sections which are probably API entries.
/^\\[a-z]*section{(al|ALLEGRO)\\_/ {
    print $0

    gsub(/\\/, "", $0)
    gsub(/^[a-z]*section/, "\\label", $0)
    print $0

    next
}

{
    # Change cross references from, to:  (notice the backslashes)
    #   \href{DUMMY_REF}{foo\_bar\_baz}
    #   \ref{foo_bar_baz}
    while (match($0, /\\href{DUMMY_REF}{[^}]*}/) > 0) {
        before = substr($0, 0, RSTART - 1)
        ref    = substr($0, RSTART, RLENGTH)
        rest   = substr($0, RSTART + RLENGTH)

        sub(/\\href{DUMMY_REF}/, "", ref)
        key = ref
        gsub(/\\/, "", key)

        # This requires pandoc --number-sections.
        printf "%s%s~(\\ref%s)", before, ref, key

        # Alternatively, we can use page numbers.
        # printf "%s%s~(pg.~\\pageref%s)", before, ref, key

        $0 = rest
    }

    print $0
}

# vim: set et:
