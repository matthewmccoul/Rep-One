# Delve Deeper: content pipeline

## Game Design Document (loaded into every session and every agent run)
The approved GDD is imported below. Every agent must treat it as required context; reading it is never optional.

@08_Final_GDD_Extracted.txt

## If you are the chat session the user is typing to
You are the relay between the user and `run_pipeline.py`, the start file in this folder. You do not run the pipeline's steps, and you never make its decisions. Only the user's own words count as their answers.

1. When the user asks you to start the pipeline, run `python3 run_pipeline.py` from this folder. It can take many minutes, so run it in the background and wait for it to finish. Then read `pipeline_state/message_for_user.md` and show the user its full text, word for word. Stop. Do not answer the question yourself, and do not replace the message with a summary of it.
2. When the user replies, write their reply, exactly as they typed it and nothing else, to `pipeline_state/reply.txt`. Then run `python3 run_pipeline.py` again and show the new `pipeline_state/message_for_user.md` the same way.
3. Repeat step 2 until the message says the pipeline is waiting on nothing or the user stops.
4. Never write `reply.txt` with words the user did not say. Never choose an option for them, fill in a missing answer, or guess what they would want. If their reply is unclear, show the message again and ask. If the user is silent, wait.
5. Never edit anything else in `pipeline_state/` (`state.json`, the task records, the logs). Never start an agent yourself with `orchestration/invoke_agent.sh`: the guard in it refuses, and the refusal is intended.
6. If `run_pipeline.py` reports a problem with its setup (for example PyYAML or `claude` missing), show the user the message and help fix that problem. Then run it again.
7. If the user asks something unrelated to the pipeline, answer it; that does not move the pipeline on.
8. When the user names data that only you can reach (their Google Drive, say) for the team to use, find it and put the copy to the user as an approval request: list each file with its name, size and where it will go (`work/inputs/`), and wait for the user's OK. Never copy before the OK. After the OK, copy each file byte for byte (decode downloads from the saved file; never retype them), check each size against the source's size, say plainly if any differs, and tell the user what is now in `work/inputs/`. Never edit the GDD or `pipeline_state/` for this.

## If you are an agent started by `run_pipeline.py` (your prompt says "You are running as agent")
- Do what your brief says, using the role file you were started with. Read the GDD parts your role file names.
- You cannot ask the user anything and you cannot record their approvals. Open questions go in `questions_for_user` or ISSUES.
- Preserve GDD intent and unresolved mechanics. Do not turn suggestions into approved game rules.
- Write only to the paths your brief names. Never write in `pipeline_state/`.
- Data the user supplied is in `work/inputs/`; your brief lists what is there. Read it before you rely on anything else.

## Known gap (user decision, 2026-11-11)
KNOWN GAP (decided by the user, 2026-11-11, hard-coded so it is not forgotten): two files of the mordor combat research are NOT in `work/inputs/` and are missing on purpose for now.
- `Mordor-Original-Manual.pdf` (9,742,019 bytes) and `Mordor-Recovered-Community-Archive.zip` (6,046,213 bytes), both in the user's Google Drive folder "Mordor Combat Research".
- Why: the Drive connector available to the chat session drops its connection on files this large (it returns the whole file as one message, and a message of about 13 MB and 8 MB was too large). The files were not copied and nothing was guessed about their contents.
- What it means: the original printed manual and the 2,591 recovered community text records are unread. Any rule that rests on them is unverified. Cite only what is in `work/inputs/`; list anything that needed those two files as a gap in your packet, never fill it from memory.
- Not forgotten means: it is re-offered to the user at the start of each task until they supply the files (they can place them in `work/inputs/` themselves) or say to stop.
