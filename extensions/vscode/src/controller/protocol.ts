import { Message } from "./process";

export namespace Protocol {
  export interface ListTests extends Message {
    action: "list";
    tests: Test[];
  }

  export function isListTests(msg: Message): msg is ListTests {
    return msg.action === "list" && Array.isArray(msg.tests);
  }

  export interface FileModified extends Message {
    action: "file_modified";
    path: string;
    dependency?: boolean;
    affected?: string[];
  }

  export interface AddWatch extends Message {
    action: "add_watch";
    path: string;
  }

  export interface TestResult extends Message {
    action: "test_result";
    results: Result[];
  }

  export function isTestResult(msg: Message): msg is TestResult {
    return msg.action === "test_result" && Array.isArray(msg.results);
  }
}

export interface Test {
  name: string;
  full_name: string[];
  cases: string[];
  path: string;
}

export interface Result {
  name: string;
  full_name: string[];
  duration: number;
  outcome: "PASS" | "FAIL" | "SKIP";
  exception: string;
}
