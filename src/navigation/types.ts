export type MainTabParamList = {
  Home: undefined;
  Map: undefined;
  Sequence: undefined;
  Settings: undefined;
};

export type RootStackParamList = {
  Login: undefined;
  SignUp: undefined;
  Scanning: undefined;
  Main: undefined;
};

declare global {
  namespace ReactNavigation {
    interface RootParamList extends RootStackParamList { }
  }
}
