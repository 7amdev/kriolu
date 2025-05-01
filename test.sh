#!/usr/bin/sh

# how diff works: https://phoenixnap.com/kb/linux-diff
# dhow redirect works: https://hwchiu.medium.com/differences-between-file-2-1-and-2-1-file-in-bash-redirection-d772660f12e2

TEST_DIR="./tests"


assert_lexer() {
    test_pass=0
    for file_test_path in $(ls $TEST_DIR/lexer/*.k)
    do 
        file_ref_name=$(basename $file_test_path .k)
        # TODO: change .ref to .expected
        file_ref_contents=$(cat $TEST_DIR/lexer/$file_ref_name.expected)

        lexer_output=$(./kriolu.exe "$file_test_path" "-lexer")
        diff_output=$(diff -c -w <(echo "$lexer_output") <(echo "$file_ref_contents")) 
        diff_status="$?"

        if [[ "$diff_status" -eq 0 ]];
        then
            echo -e "\e[32mPASSED: ${file_test_path}\e[0m" 
            test_pass=$(($test_pass + 1))
        else
            echo -e "\e[31mFAILED: ${file_test_path}\e[0m"
            echo "$diff_output"
            # echo "Expected: ${file_ref_contents}, but got  ${lexer_result}"
        fi
    done

    total=$(ls $TEST_DIR/lexer/*.k | wc -l)

    echo ""
    echo "Passed $test_pass/$total tests"
}

assert_parser() {
    test_pass=0
    for file_test_path in $(ls $TEST_DIR/parser/*.k)
    do 
        file_ref_name=$(basename $file_test_path .k)
        # TODO: change .ref to .expected
        file_ref_contents=$(cat $TEST_DIR/parser/$file_ref_name.expected)

        lexer_output=$(./kriolu.exe "$file_test_path" "-parser")
        diff_output=$(diff -c -w <(echo "$lexer_output") <(echo "$file_ref_contents")) 
        diff_status="$?"

        if [[ "$diff_status" -eq 0 ]];
        then
            echo -e "\e[32mPASSED: ${file_test_path}\e[0m" 
            test_pass=$(($test_pass + 1))
        else
            echo -e "\e[31mFAILED: ${file_test_path}\e[0m"
            echo "$diff_output"
            # echo "Expected: ${file_ref_contents}, but got  ${lexer_result}"
        fi
    done

    total=$(ls $TEST_DIR/parser/*.k | wc -l)

    echo ""
    echo "Passed $test_pass/$total tests"
}

assert_lexer
assert_parser