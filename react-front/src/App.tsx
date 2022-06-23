import React from 'react';
import logo from './logo.svg';
import styled from 'styled-components'
// import './App.css';
import SideMenu from './components/SideMenu'
import Content from './components/Content'

const Wrapper:any = styled.div`
  display:flex;
  min-height:100vh;
  width:100vw;
  background:#606060;
`

function App() {
  return (
    <Wrapper>
      <SideMenu/>
      <Content/>
    </Wrapper>
  );
}

export default App;
