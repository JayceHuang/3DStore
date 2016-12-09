package com.runmit.sweedee.manager.data;

public class DELocalData {

    //本地任务状态改变结果封装类
    public static class StructTaskState {
        public int task_id;
        public int task_state;
        public int fail_code;
        public String file_name;
        public String file_path;

        public StructTaskState(int task_id, int task_state, int fail_code, String file_name, String file_path) {
            this.task_id = task_id;
            this.task_state = task_state;
            this.fail_code = fail_code;
            this.file_name = file_name;
            this.file_path = file_path;
        }
    }
}
